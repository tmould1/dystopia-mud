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
    char keywords[MAX_INPUT_LENGTH];
    char hint[256];
    char escaped_hint[512];

    if (obj == NULL || ch == NULL || display_text == NULL)
        return display_text;

    /* If MXP not enabled, just return the display text unchanged */
    if (ch->desc == NULL || !ch->desc->mxp_enabled)
        return display_text;

    /* Use full object name for exact matching */
    if (obj->name == NULL || obj->name[0] == '\0')
        return display_text;

    /* Escape the full name for MXP */
    mxp_escape_string(keywords, obj->name, sizeof(keywords));

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

    /* Build the MXP-wrapped string based on context
     * Use single quotes around keywords for exact matching */
    if (in_room)
    {
        bool is_container = (obj->item_type == ITEM_CONTAINER
                          || obj->item_type == ITEM_CORPSE_NPC
                          || obj->item_type == ITEM_CORPSE_PC);

        if (is_container)
        {
            /* Container on ground: get/look/look in/sacrifice */
            snprintf(buf, sizeof(buf),
                MXP_SECURE_LINE "<SEND href=\"get '%s'|look '%s'|look in '%s'|sacrifice '%s'\" hint=\"%s|Get|Look|Look In|Sacrifice\">%s</SEND>" MXP_LOCK_LOCKED,
                keywords, keywords, keywords, keywords, escaped_hint, display_text);
        }
        else
        {
            /* Regular item on ground: get/look/sacrifice */
            snprintf(buf, sizeof(buf),
                MXP_SECURE_LINE "<SEND href=\"get '%s'|look '%s'|sacrifice '%s'\" hint=\"%s|Get|Look|Sacrifice\">%s</SEND>" MXP_LOCK_LOCKED,
                keywords, keywords, keywords, escaped_hint, display_text);
        }
    }
    else
    {
        /* Inventory items - build dynamic menu */
        char href_buf[2048];
        char hint_buf[1024];
        const char *use_cmd = NULL;
        const char *use_label = NULL;
        bool is_container = (obj->item_type == ITEM_CONTAINER
                          || obj->item_type == ITEM_CORPSE_NPC
                          || obj->item_type == ITEM_CORPSE_PC);
        OBJ_DATA *cont;
        char cont_keywords[MAX_INPUT_LENGTH];

        /* Priority 1: Check if item is wearable/wieldable/holdable */
        if (CAN_WEAR(obj, ITEM_WIELD))
        {
            use_cmd = "wield";
            use_label = "Wield";
        }
        else if (CAN_WEAR(obj, ITEM_HOLD))
        {
            use_cmd = "hold";
            use_label = "Hold";
        }
        else if (obj->wear_flags > ITEM_TAKE)
        {
            use_cmd = "wear";
            use_label = "Wear";
        }

        /* Priority 2: If not wearable, check consumable item types */
        if (use_cmd == NULL)
        {
            switch (obj->item_type)
            {
                case ITEM_POTION:
                    use_cmd = "quaff";
                    use_label = "Quaff";
                    break;
                case ITEM_PILL:
                case ITEM_FOOD:
                    use_cmd = "eat";
                    use_label = "Eat";
                    break;
                case ITEM_DRINK_CON:
                    use_cmd = "drink";
                    use_label = "Drink";
                    break;
                case ITEM_SCROLL:
                    use_cmd = "recite";
                    use_label = "Recite";
                    break;
                case ITEM_WAND:
                case ITEM_STAFF:
                    use_cmd = "brandish";
                    use_label = "Use";
                    break;
                default:
                    break;
            }
        }

        /* Start building href and hint strings */
        href_buf[0] = '\0';
        hint_buf[0] = '\0';

        /* Add primary action with quoted keywords */
        if (use_cmd != NULL)
        {
            snprintf(href_buf, sizeof(href_buf), "%s '%s'", use_cmd, keywords);
            snprintf(hint_buf, sizeof(hint_buf), "%s|%s", escaped_hint, use_label);
        }

        /* Add look in for containers */
        if (is_container)
        {
            if (href_buf[0] != '\0')
            {
                strcat(href_buf, "|");
                strcat(hint_buf, "|");
            }
            snprintf(href_buf + strlen(href_buf), sizeof(href_buf) - strlen(href_buf),
                "look in '%s'", keywords);
            strcat(hint_buf, "Look In");
        }

        /* Add look */
        if (href_buf[0] != '\0')
        {
            strcat(href_buf, "|");
            strcat(hint_buf, "|");
        }
        else
        {
            /* No primary action yet, start with hint */
            strcpy(hint_buf, escaped_hint);
            strcat(hint_buf, "|");
        }
        snprintf(href_buf + strlen(href_buf), sizeof(href_buf) - strlen(href_buf),
            "look '%s'", keywords);
        strcat(hint_buf, "Look");

        /* Add drop */
        strcat(href_buf, "|");
        strcat(hint_buf, "|");
        snprintf(href_buf + strlen(href_buf), sizeof(href_buf) - strlen(href_buf),
            "drop '%s'", keywords);
        strcat(hint_buf, "Drop");

        /* Add identify */
        strcat(href_buf, "|");
        strcat(hint_buf, "|");
        snprintf(href_buf + strlen(href_buf), sizeof(href_buf) - strlen(href_buf),
            "cast 'identify' '%s'", keywords);
        strcat(hint_buf, "Identify");

        /* Add "Put in <container>" for each container in player's inventory */
        if (!is_container && ch->carrying != NULL)
        {
            for (cont = ch->carrying; cont != NULL; cont = cont->next_content)
            {
                /* Skip the item itself, skip non-containers, skip closed containers */
                if (cont == obj)
                    continue;
                if (cont->item_type != ITEM_CONTAINER)
                    continue;
                if (IS_SET(cont->value[1], CONT_CLOSED))
                    continue;

                /* Get full container name for exact matching */
                if (cont->name == NULL || cont->name[0] == '\0')
                    continue;
                mxp_escape_string(cont_keywords, cont->name, sizeof(cont_keywords));

                /* Add to menu */
                strcat(href_buf, "|");
                strcat(hint_buf, "|");
                snprintf(href_buf + strlen(href_buf), sizeof(href_buf) - strlen(href_buf),
                    "put '%s' '%s'", keywords, cont_keywords);
                snprintf(hint_buf + strlen(hint_buf), sizeof(hint_buf) - strlen(hint_buf),
                    "Put in %s", cont->short_descr);
            }
        }

        /* Build final MXP tag */
        snprintf(buf, sizeof(buf),
            MXP_SECURE_LINE "<SEND href=\"%s\" hint=\"%s\">%s</SEND>" MXP_LOCK_LOCKED,
            href_buf, hint_buf, display_text);
    }

    return buf;
}

/*
 * Format an equipped item with MXP clickable link and tooltip
 * Returns a static buffer with MXP-wrapped text
 *
 * Tooltip shows item type, weight, material, and condition.
 * Menu provides remove/look options.
 */
char *mxp_equip_link(OBJ_DATA *obj, CHAR_DATA *ch, char *display_text)
{
    static char buf[MXP_BUF_MAX_LEN];
    char keywords[MAX_INPUT_LENGTH];
    char hint[256];
    char escaped_hint[512];

    if (obj == NULL || ch == NULL || display_text == NULL)
        return display_text;

    /* If MXP not enabled, just return the display text unchanged */
    if (ch->desc == NULL || !ch->desc->mxp_enabled)
        return display_text;

    /* Use full object name for more specific matching */
    if (obj->name == NULL || obj->name[0] == '\0')
        return display_text;

    /* Escape the full name for MXP - wrap in quotes if multiple words */
    mxp_escape_string(keywords, obj->name, sizeof(keywords));

    /* Build the tooltip hint with item properties */
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

        /* Show condition */
        if (obj->condition >= 100)
            snprintf(condition, sizeof(condition), ", Perfect");
        else if (obj->condition >= 75)
            snprintf(condition, sizeof(condition), ", %d%% Good", obj->condition);
        else if (obj->condition >= 50)
            snprintf(condition, sizeof(condition), ", %d%% Worn", obj->condition);
        else if (obj->condition >= 25)
            snprintf(condition, sizeof(condition), ", %d%% Damaged", obj->condition);
        else
            snprintf(condition, sizeof(condition), ", %d%% Broken", obj->condition);

        snprintf(hint, sizeof(hint), "%s, %dlb%s%s",
            item_type_name(obj), obj->weight, material, condition);
    }

    /* Escape the hint for MXP */
    mxp_escape_string(escaped_hint, hint, sizeof(escaped_hint));

    /* Build the MXP-wrapped string: remove/look
     * Use single quotes around keywords to handle multi-word names like 'newbie sword' */
    snprintf(buf, sizeof(buf),
        MXP_SECURE_LINE "<SEND href=\"remove '%s'|look '%s'\" hint=\"%s|Remove|Look\">%s</SEND>" MXP_LOCK_LOCKED,
        keywords, keywords, escaped_hint, display_text);

    return buf;
}

/*
 * Format an item inside a container with MXP clickable link
 * Returns a static buffer with MXP-wrapped text
 *
 * Click runs 'get <item> <container>' to take the item out.
 * Tooltip shows item type, weight, and condition.
 */
char *mxp_container_item_link(OBJ_DATA *obj, OBJ_DATA *container, CHAR_DATA *ch, char *display_text)
{
    static char buf[MXP_BUF_MAX_LEN];
    char obj_keywords[MAX_INPUT_LENGTH];
    char container_keywords[MAX_INPUT_LENGTH];
    char hint[256];
    char escaped_hint[512];

    if (obj == NULL || container == NULL || ch == NULL || display_text == NULL)
        return display_text;

    /* If MXP not enabled, just return the display text unchanged */
    if (ch->desc == NULL || !ch->desc->mxp_enabled)
        return display_text;

    /* Get full object name for exact matching */
    if (obj->name == NULL || obj->name[0] == '\0')
        return display_text;
    mxp_escape_string(obj_keywords, obj->name, sizeof(obj_keywords));

    /* Get full container name for exact matching */
    if (container->name == NULL || container->name[0] == '\0')
        return display_text;
    mxp_escape_string(container_keywords, container->name, sizeof(container_keywords));

    /* Build the tooltip hint */
    {
        char material[64] = "";
        char condition[32] = "";

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

        if (obj->condition < 100)
            snprintf(condition, sizeof(condition), ", %d%% condition", obj->condition);

        snprintf(hint, sizeof(hint), "%s, Weight: %d%s%s",
            item_type_name(obj), obj->weight, material, condition);
    }

    mxp_escape_string(escaped_hint, hint, sizeof(escaped_hint));

    /* Simple click to get item from container - no menu needed */
    snprintf(buf, sizeof(buf),
        MXP_SECURE_LINE "<SEND href=\"get '%s' '%s'\" hint=\"%s\">%s</SEND>" MXP_LOCK_LOCKED,
        obj_keywords, container_keywords, escaped_hint, display_text);

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
        /* Start with room name */
        snprintf(hint, sizeof(hint), "%s: ", to_room->name ? to_room->name : "Unknown");

        /* Build list of visible characters - players first, then NPCs */
        /* First pass: count and add players */
        for (vch = to_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (IS_NPC(vch))
                continue;
            if (!can_see(ch, vch))
                continue;

            if (count >= 3)
            {
                strncat(hint, ", ...", sizeof(hint) - strlen(hint) - 1);
                count++;
                break;
            }

            if (count > 0)
                strncat(hint, ", ", sizeof(hint) - strlen(hint) - 1);

            if (IS_AFFECTED(vch, AFF_POLYMORPH) && vch->morph != NULL)
                strncat(hint, vch->morph, sizeof(hint) - strlen(hint) - 1);
            else
                strncat(hint, vch->name, sizeof(hint) - strlen(hint) - 1);
            count++;
        }

        /* Second pass: add NPCs if we haven't hit the limit */
        if (count <= 3)
        {
            for (vch = to_room->people; vch != NULL; vch = vch->next_in_room)
            {
                if (!IS_NPC(vch))
                    continue;
                if (!can_see(ch, vch))
                    continue;

                if (count >= 3)
                {
                    strncat(hint, ", ...", sizeof(hint) - strlen(hint) - 1);
                    count++;
                    break;
                }

                if (count > 0)
                    strncat(hint, ", ", sizeof(hint) - strlen(hint) - 1);

                strncat(hint, vch->short_descr, sizeof(hint) - strlen(hint) - 1);
                count++;
            }
        }

        if (count == 0)
            strncat(hint, "Nobody here", sizeof(hint) - strlen(hint) - 1);
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

/*
 * Wrap a status prefix with MXP hover tooltip
 *
 * ch      - viewer (to check spell knowledge and MXP status)
 * prefix  - the display text like "#C(White Aura)#n "
 * tooltip - hover text like "Sanctuary"
 * sn      - spell number to check knowledge, or -1 for no check
 *
 * Returns MXP-wrapped prefix if:
 *   - MXP is enabled
 *   - sn == -1 (no spell check) OR viewer knows the spell
 *
 * If spell unknown, shows "Unknown spell" as tooltip instead.
 * If MXP disabled, returns prefix unchanged.
 */
char *mxp_aura_tag(CHAR_DATA *ch, const char *prefix, const char *tooltip, int sn)
{
    static char bufs[16][512];
    static int idx = 0;
    char *buf;
    const char *display_tooltip;
    char escaped_tooltip[256];

    if (ch == NULL || prefix == NULL)
        return (char *)prefix;

    /* If MXP not enabled, just return the prefix unchanged */
    if (ch->desc == NULL || !ch->desc->mxp_enabled)
        return (char *)prefix;

    /* Rotate through static buffers */
    buf = bufs[idx];
    idx = (idx + 1) % 16;

    /* Determine tooltip based on spell knowledge */
    if (sn >= 0)
    {
        /* Spell-based prefix - check if viewer knows the spell */
        if (IS_NPC(ch) || ch->pcdata->learned[sn] <= 0)
            display_tooltip = "Unknown spell";
        else
            display_tooltip = tooltip;
    }
    else
    {
        /* Non-spell prefix - always show the tooltip */
        display_tooltip = tooltip;
    }

    /* Escape the tooltip for MXP */
    mxp_escape_string(escaped_tooltip, display_tooltip, sizeof(escaped_tooltip));

    /* Build MXP-wrapped prefix with hover-only tooltip (no href = not clickable)
     * Strip trailing space from prefix and add it after the tag for clean boundaries */
    {
        char clean_prefix[256];
        size_t len;
        bool had_trailing_space = FALSE;

        strncpy(clean_prefix, prefix, sizeof(clean_prefix) - 1);
        clean_prefix[sizeof(clean_prefix) - 1] = '\0';
        len = strlen(clean_prefix);

        /* Strip trailing space */
        if (len > 0 && clean_prefix[len - 1] == ' ')
        {
            clean_prefix[len - 1] = '\0';
            had_trailing_space = TRUE;
        }

        snprintf(buf, 512,
            MXP_SECURE_LINE "<SEND hint=\"%s\">%s</SEND>" MXP_LOCK_LOCKED "%s",
            escaped_tooltip, clean_prefix, had_trailing_space ? " " : "");
    }

    return buf;
}
