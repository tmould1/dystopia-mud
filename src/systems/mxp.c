/*
 * mxp.c - MUD eXtension Protocol implementation
 *
 * Provides MXP support for rich text features in compatible clients.
 * Follows the MCCP pattern for protocol negotiation.
 */

#include "../core/merc.h"
#include "telnet.h"
#include "mxp.h"

/* External function from comm.c */
bool write_to_descriptor args( ( DESCRIPTOR_DATA *d, char *txt, int length ) );

/*
 * Telnet negotiation sequences for MXP
 * These are extern'd and used in comm.c for protocol negotiation.
 */
#if defined(unix)
const char mxp_will[] = { IAC, WILL, TELOPT_MXP, '\0' };
const char mxp_wont[] = { IAC, WONT, TELOPT_MXP, '\0' };
const char mxp_do[]   = { IAC, DO, TELOPT_MXP, '\0' };
const char mxp_dont[] = { IAC, DONT, TELOPT_MXP, '\0' };
const char mxp_sb[]   = { IAC, SB, TELOPT_MXP, IAC, SE, '\0' };
#endif

#if defined(WIN32)
const char mxp_will[] = { (char)IAC, (char)WILL, (char)TELOPT_MXP, '\0' };
const char mxp_wont[] = { (char)IAC, (char)WONT, (char)TELOPT_MXP, '\0' };
const char mxp_do[]   = { (char)IAC, (char)DO, (char)TELOPT_MXP, '\0' };
const char mxp_dont[] = { (char)IAC, (char)DONT, (char)TELOPT_MXP, '\0' };
const char mxp_sb[]   = { (char)IAC, (char)SB, (char)TELOPT_MXP, (char)IAC, (char)SE, '\0' };
#endif

#if defined(macintosh) || defined(MSDOS)
const char mxp_will[] = { '\0' };
const char mxp_wont[] = { '\0' };
const char mxp_do[]   = { '\0' };
const char mxp_dont[] = { '\0' };
const char mxp_sb[]   = { '\0' };
#endif

/*
 * MXP mode escape sequences (ESC[#z)
 * 0 = Open line, 1 = Secure line, 2 = Locked line
 * 5 = Lock open, 6 = Lock secure, 7 = Lock locked (default)
 */
static const char mxp_lock_locked[] = "\033[7z";  /* Set default to locked mode */

/*
 * Start MXP mode on a descriptor
 * Called when client responds with IAC DO MXP
 *
 * Per MXP spec, after client sends IAC DO MXP, we must send:
 * IAC SB MXP IAC SE to tell the client MXP mode is now active.
 * Then we set locked mode as default so normal text isn't parsed.
 */
bool mxpStart(DESCRIPTOR_DATA *desc)
{
    if (desc == NULL)
        return FALSE;

    if (desc->mxp_enabled)
        return TRUE;  /* Already enabled */

    /* Send the MXP subnegotiation start sequence */
    write_to_descriptor(desc, (char *)mxp_sb, strlen(mxp_sb));

    /* Set default mode to LOCKED so normal text isn't parsed as MXP.
     * This prevents raw < and > from being interpreted as tags.
     * Use #M escape in text to switch to secure mode for actual MXP tags. */
    write_to_descriptor(desc, (char *)mxp_lock_locked, strlen(mxp_lock_locked));

    desc->mxp_enabled = TRUE;
    return TRUE;
}

/*
 * End MXP mode on a descriptor
 * Called when client sends IAC DONT MXP or on disconnect
 */
void mxpEnd(DESCRIPTOR_DATA *desc)
{
    if (desc == NULL)
        return;

    desc->mxp_enabled = FALSE;
}

/*
 * User command to check MXP status
 */
void do_mxp(CHAR_DATA *ch, char *argument)
{
    if (ch == NULL)
        return;

    if (ch->desc == NULL)
    {
        send_to_char("You have no descriptor!\n\r", ch);
        return;
    }

    if (ch->desc->mxp_enabled)
    {
        send_to_char("MXP is enabled for your connection.\n\r", ch);
        send_to_char("Your client supports the MUD eXtension Protocol.\n\r", ch);
    }
    else
    {
        send_to_char("MXP is not enabled for your connection.\n\r", ch);
        send_to_char("Your client either doesn't support MXP or declined the negotiation.\n\r", ch);
    }
}

/*
 * Escape special characters for MXP attributes
 * < > " & must be escaped in MXP tag attributes
 */
void mxp_escape_string(char *dest, const char *src, size_t maxlen)
{
    size_t i = 0;

    if (dest == NULL || src == NULL || maxlen == 0)
        return;

    while (*src && i < maxlen - 1)
    {
        switch (*src)
        {
            case '<':
                if (i + 4 < maxlen) { strcpy(&dest[i], "&lt;"); i += 4; }
                break;
            case '>':
                if (i + 4 < maxlen) { strcpy(&dest[i], "&gt;"); i += 4; }
                break;
            case '"':
                if (i + 6 < maxlen) { strcpy(&dest[i], "&quot;"); i += 6; }
                break;
            case '&':
                if (i + 5 < maxlen) { strcpy(&dest[i], "&amp;"); i += 5; }
                break;
            default:
                dest[i++] = *src;
                break;
        }
        src++;
    }
    dest[i] = '\0';
}

/*
 * Format an object with MXP clickable link and tooltip
 * Returns a static buffer with MXP-wrapped text
 *
 * in_room = TRUE:  Item is on the ground - click to get, menu for look/get
 * in_room = FALSE: Item is in inventory - click to identify
 *
 * Tooltip shows item type, weight, material, and condition.
 */
char *mxp_obj_link(OBJ_DATA *obj, CHAR_DATA *ch, char *display_text, bool in_room)
{
    static char buf[MXP_BUF_MAX_LEN];
    char keyword[MAX_INPUT_LENGTH];
    char hint[256];
    char escaped_hint[512];
    char arg[MAX_INPUT_LENGTH];

    if (obj == NULL || ch == NULL || display_text == NULL)
        return display_text;

    /* If MXP not enabled, just return the display text unchanged */
    if (ch->desc == NULL || !ch->desc->mxp_enabled)
        return display_text;

    /* Get the first keyword from the object name */
    one_argument(obj->name, arg);
    if (arg[0] == '\0')
        return display_text;

    /* Escape the keyword for MXP */
    mxp_escape_string(keyword, arg, sizeof(keyword));

    /* Build the tooltip hint - visible item properties */
    {
        char material[64] = "";
        char condition[32] = "";

        /* Determine material if visible */
        if (IS_SET(obj->spectype, SITEM_ADAMANTITE))
            strcpy(material, ", Adamantite");
        else if (IS_SET(obj->spectype, SITEM_STEEL))
            strcpy(material, ", Steel");
        else if (IS_SET(obj->spectype, SITEM_IRON))
            strcpy(material, ", Iron");
        else if (IS_SET(obj->spectype, SITEM_COPPER))
            strcpy(material, ", Copper");
        else if (IS_SET(obj->spectype, SITEM_DEMONIC))
            strcpy(material, ", Demonsteel");
        else if (IS_SET(obj->spectype, SITEM_SILVER))
            strcpy(material, ", Silver");

        /* Show condition if damaged */
        if (obj->condition < 100)
            snprintf(condition, sizeof(condition), ", %d%% condition", obj->condition);

        snprintf(hint, sizeof(hint), "%s, Weight: %d%s%s",
            item_type_name(obj), obj->weight, material, condition);
    }

    /* Escape the hint for MXP */
    mxp_escape_string(escaped_hint, hint, sizeof(escaped_hint));

    /* Build the MXP-wrapped string based on context */
    if (in_room)
    {
        /* Item on the ground: left-click = get, right-click menu = get/look/sacrifice
         * Using N+1 hint format: first hint is tooltip, rest are menu labels */
        snprintf(buf, sizeof(buf),
            MXP_SECURE_LINE "<SEND href=\"get %s|look %s|sacrifice %s\" hint=\"%s|Get|Look|Sacrifice\">%s</SEND>" MXP_LOCK_LOCKED,
            keyword, keyword, keyword, escaped_hint, display_text);
    }
    else
    {
        /* Item in inventory: left-click = identify, right-click = look/drop/identify
         * Using N+1 hint format: first hint is tooltip, rest are menu labels */
        snprintf(buf, sizeof(buf),
            MXP_SECURE_LINE "<SEND href=\"cast 'identify' %s|look %s|drop %s\" hint=\"%s|Identify|Look|Drop\">%s</SEND>" MXP_LOCK_LOCKED,
            keyword, keyword, keyword, escaped_hint, display_text);
    }

    return buf;
}

/*
 * Format a player name with MXP clickable link and tooltip
 * Returns a static buffer with MXP-wrapped text
 *
 * Click runs 'finger <name>', tooltip shows PK stats
 */
char *mxp_player_link(CHAR_DATA *victim, CHAR_DATA *ch, char *display_text)
{
    static char buf[MXP_BUF_MAX_LEN];
    char hint[128];
    char escaped_hint[256];
    char escaped_name[64];
    char *name;
    int pk_ratio;

    if (victim == NULL || ch == NULL || display_text == NULL)
        return display_text;

    /* If MXP not enabled, just return the display text unchanged */
    if (ch->desc == NULL || !ch->desc->mxp_enabled)
        return display_text;

    /* Get the player's name */
    if (IS_NPC(victim) || victim->pcdata == NULL)
        return display_text;

    name = victim->pcdata->switchname;
    if (name == NULL || name[0] == '\0')
        return display_text;

    /* Calculate PK ratio for tooltip */
    if (victim->pkill > 0 && (victim->pkill + victim->pdeath) > 0)
        pk_ratio = (100 * victim->pkill) / (victim->pkill + victim->pdeath);
    else
        pk_ratio = 0;

    /* Build the tooltip hint - PK stats */
    snprintf(hint, sizeof(hint), "Kills: %d, Deaths: %d (%.2f ratio)",
        victim->pkill, victim->pdeath, pk_ratio / 100.0);

    /* Escape for MXP */
    mxp_escape_string(escaped_name, name, sizeof(escaped_name));
    mxp_escape_string(escaped_hint, hint, sizeof(escaped_hint));

    /* Build the MXP-wrapped string */
    snprintf(buf, sizeof(buf),
        MXP_SECURE_LINE "<SEND href=\"finger %s\" hint=\"%s\">%s</SEND>" MXP_LOCK_LOCKED,
        escaped_name, escaped_hint, display_text);

    return buf;
}

/*
 * Format an exit direction with MXP clickable link and tooltip
 * Returns a static buffer with MXP-wrapped text
 *
 * Click action is context-aware:
 * - Closed door (and can't pass): "open <dir>" to open the door
 * - Otherwise: "<dir>" to move in that direction
 *
 * Tooltip shows preview of what's there
 */
char *mxp_exit_link(EXIT_DATA *pexit, int door, CHAR_DATA *ch, char *display_text)
{
    static char buf[MXP_BUF_MAX_LEN];
    static const char *dir_cmd[] = { "north", "east", "south", "west", "up", "down" };
    char hint[256];
    char escaped_hint[512];
    char cmd[64];
    ROOM_INDEX_DATA *to_room;
    CHAR_DATA *vch;
    int count = 0;
    bool door_blocks = FALSE;

    if (pexit == NULL || ch == NULL || display_text == NULL)
        return display_text;

    /* If MXP not enabled, just return the display text unchanged */
    if (ch->desc == NULL || !ch->desc->mxp_enabled)
        return display_text;

    /* Validate door index */
    if (door < 0 || door > 5)
        return display_text;

    to_room = pexit->to_room;
    if (to_room == NULL)
        return display_text;

    /* Check if closed door blocks movement */
    if (IS_SET(pexit->exit_info, EX_CLOSED)
        && !IS_AFFECTED(ch, AFF_PASS_DOOR)
        && !IS_AFFECTED(ch, AFF_ETHEREAL)
        && !IS_AFFECTED(ch, AFF_SHADOWPLANE))
    {
        door_blocks = TRUE;
    }

    /* Build tooltip hint based on what's beyond */
    if (IS_SET(pexit->exit_info, EX_CLOSED))
    {
        if (IS_SET(pexit->exit_info, EX_LOCKED))
            snprintf(hint, sizeof(hint), "Locked door");
        else
            snprintf(hint, sizeof(hint), "Closed door");
    }
    else if (room_is_dark(to_room) && !IS_AFFECTED(ch, AFF_INFRARED)
             && !IS_SET(ch->act, PLR_HOLYLIGHT))
    {
        snprintf(hint, sizeof(hint), "Too dark to see");
    }
    else
    {
        /* Build list of visible characters in the room */
        hint[0] = '\0';

        for (vch = to_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!can_see(ch, vch))
                continue;

            if (count >= 3)
            {
                /* Too many to list, just indicate more */
                strcat(hint, ", ...");
                break;
            }

            if (count > 0)
                strcat(hint, ", ");

            if (IS_NPC(vch))
            {
                /* Show NPC short description */
                strncat(hint, vch->short_descr, sizeof(hint) - strlen(hint) - 1);
            }
            else if (IS_AFFECTED(vch, AFF_POLYMORPH))
            {
                strncat(hint, vch->morph, sizeof(hint) - strlen(hint) - 1);
            }
            else
            {
                strncat(hint, vch->name, sizeof(hint) - strlen(hint) - 1);
            }
            count++;
        }

        if (count == 0)
            snprintf(hint, sizeof(hint), "Nobody here");
    }

    /* Escape the hint for MXP */
    mxp_escape_string(escaped_hint, hint, sizeof(escaped_hint));

    /* Build the command - open door if closed, otherwise move */
    if (door_blocks)
        snprintf(cmd, sizeof(cmd), "open %s", dir_cmd[door]);
    else
        snprintf(cmd, sizeof(cmd), "%s", dir_cmd[door]);

    /* Build the MXP-wrapped string */
    snprintf(buf, sizeof(buf),
        MXP_SECURE_LINE "<SEND href=\"%s\" hint=\"%s\">%s</SEND>" MXP_LOCK_LOCKED,
        cmd, escaped_hint, display_text);

    return buf;
}

/*
 * Get health status string for a character
 * Returns a static string describing their health percentage
 */
static const char *get_health_string(CHAR_DATA *victim)
{
    int percent;

    if (victim->max_hit > 0)
        percent = (100 * victim->hit) / victim->max_hit;
    else
        percent = -1;

    if (percent >= 100) return "is in perfect health";
    if (percent >= 90)  return "is slightly scratched";
    if (percent >= 80)  return "has a few bruises";
    if (percent >= 70)  return "has some cuts";
    if (percent >= 60)  return "has several wounds";
    if (percent >= 50)  return "has many nasty wounds";
    if (percent >= 40)  return "is bleeding freely";
    if (percent >= 30)  return "is covered in blood";
    if (percent >= 20)  return "is leaking guts";
    if (percent >= 10)  return "is almost dead";
    return "is DYING";
}

/*
 * Format a character (NPC or PC) with MXP clickable link and tooltip
 * Returns a static buffer with MXP-wrapped text
 *
 * Left-click runs 'consider <target>', right-click runs 'attack <target>'
 * Tooltip shows health status
 */
char *mxp_char_link(CHAR_DATA *victim, CHAR_DATA *ch, char *display_text)
{
    static char buf[MXP_BUF_MAX_LEN];
    char hint[256];
    char escaped_hint[512];
    char keyword[MAX_INPUT_LENGTH];
    char escaped_keyword[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char clean_text[MAX_STRING_LENGTH];
    char trailing[8] = "";

    if (victim == NULL || ch == NULL || display_text == NULL)
        return display_text;

    /* If MXP not enabled, just return the display text unchanged */
    if (ch->desc == NULL || !ch->desc->mxp_enabled)
        return display_text;

    /* Don't link to self */
    if (victim == ch)
        return display_text;

    /* Copy display text and strip trailing newlines - MXP tags must be on same line */
    strncpy(clean_text, display_text, sizeof(clean_text) - 1);
    clean_text[sizeof(clean_text) - 1] = '\0';

    /* Find and save trailing newlines, then strip them */
    {
        size_t len = strlen(clean_text);
        size_t trail_start = len;

        /* Find where trailing newlines begin */
        while (trail_start > 0 && (clean_text[trail_start-1] == '\n' || clean_text[trail_start-1] == '\r'))
            trail_start--;

        /* Copy trailing chars (max 4 chars like \n\r\n\r) */
        if (trail_start < len)
        {
            size_t trail_len = len - trail_start;
            if (trail_len > sizeof(trailing) - 1)
                trail_len = sizeof(trailing) - 1;
            memcpy(trailing, &clean_text[trail_start], trail_len);
            trailing[trail_len] = '\0';
            clean_text[trail_start] = '\0';
        }
    }

    /* Get keyword to target this character */
    if (IS_NPC(victim))
    {
        /* Use first keyword from NPC name */
        one_argument(victim->name, arg);
        if (arg[0] == '\0')
            return display_text;
        snprintf(keyword, sizeof(keyword), "%s", arg);
    }
    else
    {
        /* Use player name */
        if (IS_AFFECTED(victim, AFF_POLYMORPH) && victim->morph != NULL)
        {
            /* Polymorphed - use morph name for targeting */
            one_argument(victim->morph, arg);
            if (arg[0] == '\0')
                snprintf(keyword, sizeof(keyword), "%s", victim->name);
            else
                snprintf(keyword, sizeof(keyword), "%s", arg);
        }
        else
        {
            snprintf(keyword, sizeof(keyword), "%s", victim->name);
        }
    }

    /* Build the tooltip hint - include short name and health status */
    if (IS_NPC(victim))
        snprintf(hint, sizeof(hint), "%s %s", victim->short_descr, get_health_string(victim));
    else if (IS_AFFECTED(victim, AFF_POLYMORPH) && victim->morph != NULL)
        snprintf(hint, sizeof(hint), "%s %s", victim->morph, get_health_string(victim));
    else
        snprintf(hint, sizeof(hint), "%s %s", victim->name, get_health_string(victim));

    /* Escape for MXP */
    mxp_escape_string(escaped_keyword, keyword, sizeof(escaped_keyword));
    mxp_escape_string(escaped_hint, hint, sizeof(escaped_hint));

    /* Build the MXP-wrapped string:
     * Left-click (first href) = attack
     * Right-click menu (after |) = look, consider, and finger for players
     *
     * Mudlet hint behavior: if there is ONE EXTRA hint compared to commands,
     * the first hint becomes the hover tooltip and the rest are menu labels.
     * So: href has N commands, hint has N+1 entries (tooltip + N menu labels)
     *
     * Trailing newlines are added after the MXP closing tag */
    if (IS_NPC(victim))
    {
        /* 3 commands: attack, look, consider -> 4 hints: tooltip + 3 labels */
        snprintf(buf, sizeof(buf),
            MXP_SECURE_LINE "<SEND href=\"attack %s|look %s|consider %s\" hint=\"%s|Attack|Look|Consider\">%s</SEND>" MXP_LOCK_LOCKED "%s",
            escaped_keyword, escaped_keyword, escaped_keyword, escaped_hint, clean_text, trailing);
    }
    else
    {
        /* 4 commands: attack, look, consider, finger -> 5 hints: tooltip + 4 labels */
        snprintf(buf, sizeof(buf),
            MXP_SECURE_LINE "<SEND href=\"attack %s|look %s|consider %s|finger %s\" hint=\"%s|Attack|Look|Consider|Finger\">%s</SEND>" MXP_LOCK_LOCKED "%s",
            escaped_keyword, escaped_keyword, escaped_keyword, escaped_keyword, escaped_hint, clean_text, trailing);
    }

    return buf;
}
