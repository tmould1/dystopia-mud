/*
 * mxp.c - MUD eXtension Protocol implementation
 *
 * Provides MXP support for rich text features in compatible clients.
 * Follows the MCCP pattern for protocol negotiation.
 *
 * Note: Forge-able items (copper, iron, steel, adamantite, gemstone, hilt)
 * display "Forge onto <item>" options in their MXP tooltip menu for each
 * valid target item in the player's inventory. Hilts only apply to weapons;
 * metals and gems apply to weapons and armor.
 */

#include <string.h>
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
    write_to_descriptor(desc, (char *)mxp_sb, (int)strlen(mxp_sb));

    /* Set default mode to LOCKED so normal text isn't parsed as MXP.
     * This prevents raw < and > from being interpreted as tags.
     * Use #M escape in text to switch to secure mode for actual MXP tags. */
    write_to_descriptor(desc, (char *)mxp_lock_locked, (int)strlen(mxp_lock_locked));

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
 * Strip MUD color codes from a string
 * Color codes are #X where X is a digit (0-9) or letter (R,G,B,Y,C,M,W,L,n,x,etc)
 * ## produces a literal #, #- produces a literal - (consistent with col_str_len)
 * Used to clean strings for MXP attributes
 */
static void mxp_strip_colors(char *dest, const char *src, size_t maxlen)
{
    size_t i = 0;

    if (dest == NULL || src == NULL || maxlen == 0)
        return;

    while (*src && i < maxlen - 1)
    {
        if (*src == '#' && *(src + 1) != '\0')
        {
            src++;  /* Skip the # */
            /* ## and #- are literal characters */
            if (*src == '#' || *src == '-')
                dest[i++] = *src;
            src++;  /* Skip the color code character */
            continue;
        }
        dest[i++] = *src++;
    }
    dest[i] = '\0';
}

/*
 * Escape special characters for MXP attributes
 * < > " & must be escaped in MXP tag attributes
 * Also filter out control characters that could break tags
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
            case '\n':
            case '\r':
            case '\033':  /* ESC - could break MXP mode tags */
                /* Skip control characters that could break MXP tags */
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
 * Extract spell names from items that cast spells (potions, scrolls, pills, wands, staffs)
 * Writes comma-separated spell names to buf
 */
static void mxp_get_spell_names(OBJ_DATA *obj, char *buf, size_t maxlen)
{
    char temp[128];
    int first = 1;

    if (buf == NULL || maxlen == 0)
        return;

    buf[0] = '\0';

    /* For potions/pills/scrolls: value[1], value[2], value[3] are spell slots */
    if (obj->item_type == ITEM_POTION || obj->item_type == ITEM_PILL || obj->item_type == ITEM_SCROLL)
    {
        if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL && skill_table[obj->value[1]].name != NULL)
        {
            snprintf(temp, sizeof(temp), "'%s'", skill_table[obj->value[1]].name);
            strncat(buf, temp, maxlen - strlen(buf) - 1);
            first = 0;
        }
        if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL && skill_table[obj->value[2]].name != NULL)
        {
            snprintf(temp, sizeof(temp), "%s'%s'", first ? "" : " ", skill_table[obj->value[2]].name);
            strncat(buf, temp, maxlen - strlen(buf) - 1);
            first = 0;
        }
        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL && skill_table[obj->value[3]].name != NULL)
        {
            snprintf(temp, sizeof(temp), "%s'%s'", first ? "" : " ", skill_table[obj->value[3]].name);
            strncat(buf, temp, maxlen - strlen(buf) - 1);
        }
    }
    /* For wands/staffs: value[3] is the spell */
    else if (obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF)
    {
        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL && skill_table[obj->value[3]].name != NULL)
        {
            snprintf(buf, maxlen, "'%s'", skill_table[obj->value[3]].name);
        }
    }
}

/*
 * Get the destination room name for a portal or wgate
 * Returns static string with room name or "Unknown"
 */
static const char *mxp_get_portal_destination(OBJ_DATA *obj)
{
    ROOM_INDEX_DATA *room;

    if (obj == NULL || obj->value[0] <= 0)
        return "Unknown";

    room = get_room_index(obj->value[0]);
    if (room == NULL)
        return "Nowhere";

    return room->name ? room->name : "Unknown";
}

/*
 * Build enhanced tooltip based on item type
 * Includes item-specific information like spells, charges, destinations, etc.
 */
static void mxp_build_item_tooltip(OBJ_DATA *obj, CHAR_DATA *ch, char *hint, size_t maxlen)
{
    char material[64] = "";
    char condition[32] = "";
    char extra[384] = "";
    char spells[256];

    if (obj == NULL || hint == NULL || maxlen == 0)
        return;

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
        snprintf(condition, sizeof(condition), ", %d%%", obj->condition);

    /* Item-type specific extras */
    switch (obj->item_type)
    {
        case ITEM_POTION:
        case ITEM_PILL:
        case ITEM_SCROLL:
            mxp_get_spell_names(obj, spells, sizeof(spells));
            if (spells[0] != '\0')
                snprintf(extra, sizeof(extra), ", %s Lvl %d", spells, obj->value[0]);
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            mxp_get_spell_names(obj, spells, sizeof(spells));
            if (spells[0] != '\0')
                snprintf(extra, sizeof(extra), ", %d/%d %s", obj->value[2], obj->value[1], spells);
            else
                snprintf(extra, sizeof(extra), ", %d/%d charges", obj->value[2], obj->value[1]);
            break;

        case ITEM_PORTAL:
        case ITEM_WGATE:
            snprintf(extra, sizeof(extra), ", To: %s", mxp_get_portal_destination(obj));
            break;

        case ITEM_FOUNTAIN:
            if (obj->value[2] >= 0 && obj->value[2] < LIQ_MAX)
                snprintf(extra, sizeof(extra), ", %s", liq_table[obj->value[2]].liq_name);
            break;

        case ITEM_DRINK_CON:
            if (obj->value[1] <= 0)
                strcpy(extra, ", Empty");
            else if (obj->value[2] >= 0 && obj->value[2] < LIQ_MAX)
                snprintf(extra, sizeof(extra), ", %d/%d %s",
                    obj->value[1], obj->value[0], liq_table[obj->value[2]].liq_name);
            break;

        case ITEM_LIGHT:
            if (obj->value[2] == -1)
                strcpy(extra, ", Infinite");
            else if (obj->value[2] > 0)
                snprintf(extra, sizeof(extra), ", %d hrs", obj->value[2]);
            else
                strcpy(extra, ", Burned out");
            break;

        case ITEM_FOOD:
            if (obj->value[3] != 0)
                snprintf(extra, sizeof(extra), ", Fills %d (Poisoned!)", obj->value[0]);
            else
                snprintf(extra, sizeof(extra), ", Fills %d", obj->value[0]);
            break;

        case ITEM_QUEST:
            snprintf(extra, sizeof(extra), ", %d QP", obj->value[0]);
            break;

        case ITEM_QUESTCARD:
            snprintf(extra, sizeof(extra), ", Reward: %d QP", obj->level);
            break;

        case ITEM_MONEY:
            snprintf(extra, sizeof(extra), ", %d gold", obj->value[0]);
            break;

        case ITEM_AMMO:
            snprintf(extra, sizeof(extra), ", Dmg %d-%d", obj->value[1], obj->value[2]);
            break;

        case ITEM_MISSILE:
            snprintf(extra, sizeof(extra), ", %d/%d loaded", obj->value[1], obj->value[2]);
            break;

        case ITEM_ARMOR:
            snprintf(extra, sizeof(extra), ", AC %d", obj->value[0]);
            break;

        case ITEM_WEAPON:
            snprintf(extra, sizeof(extra), ", %dd%d", obj->value[1], obj->value[2]);
            break;

        default:
            break;
    }

    snprintf(hint, maxlen, "%s, Wt: %d%s%s%s",
        item_type_name(obj), obj->weight, material, condition, extra);
}

/*
 * Build MXP menu for items on the ground (in_room = TRUE)
 * Context-aware primary action based on item type
 */
static void mxp_build_ground_menu(OBJ_DATA *obj, CHAR_DATA *ch,
                                   const char *keywords, const char *escaped_hint,
                                   char *href_buf, size_t href_size,
                                   char *hint_buf, size_t hint_size)
{
    if (obj == NULL || keywords == NULL)
        return;

    href_buf[0] = '\0';
    hint_buf[0] = '\0';

    switch (obj->item_type)
    {
        case ITEM_PORTAL:
        case ITEM_WGATE:
            snprintf(href_buf, href_size, "enter '%s'|look '%s'", keywords, keywords);
            snprintf(hint_buf, hint_size, "%s|Enter|Look", escaped_hint);
            break;

        case ITEM_FOUNTAIN:
            {
                /* Check if player has any drink containers to fill */
                OBJ_DATA *drink_con;
                bool has_container = FALSE;

                for (drink_con = ch->carrying; drink_con != NULL; drink_con = drink_con->next_content)
                {
                    if (drink_con->item_type == ITEM_DRINK_CON)
                    {
                        has_container = TRUE;
                        break;
                    }
                }

                if (has_container)
                {
                    snprintf(href_buf, href_size, "drink '%s'|fill '%s'|look '%s'",
                        keywords, keywords, keywords);
                    snprintf(hint_buf, hint_size, "%s|Drink|Fill|Look", escaped_hint);
                }
                else
                {
                    snprintf(href_buf, href_size, "drink '%s'|look '%s'",
                        keywords, keywords);
                    snprintf(hint_buf, hint_size, "%s|Drink|Look", escaped_hint);
                }
            }
            break;

        case ITEM_FURNITURE:
            snprintf(href_buf, href_size, "sit '%s'|rest '%s'|sleep '%s'|look '%s'",
                keywords, keywords, keywords, keywords);
            snprintf(hint_buf, hint_size, "%s|Sit|Rest|Sleep|Look", escaped_hint);
            break;

        case ITEM_QUESTMACHINE:
            snprintf(href_buf, href_size, "request|complete|look '%s'", keywords);
            snprintf(hint_buf, hint_size, "%s|Request|Complete|Look", escaped_hint);
            break;

        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            snprintf(href_buf, href_size, "get '%s'|look '%s'|look in '%s'|sacrifice '%s'",
                keywords, keywords, keywords, keywords);
            snprintf(hint_buf, hint_size, "%s|Get|Look|Look In|Sacrifice", escaped_hint);
            break;

        case ITEM_MONEY:
            snprintf(href_buf, href_size, "get '%s'|look '%s'", keywords, keywords);
            snprintf(hint_buf, hint_size, "%s|Get|Look", escaped_hint);
            break;

        default:
            /* Standard ground item: get/look/sacrifice */
            snprintf(href_buf, href_size, "get '%s'|look '%s'|sacrifice '%s'",
                keywords, keywords, keywords);
            snprintf(hint_buf, hint_size, "%s|Get|Look|Sacrifice", escaped_hint);
            break;
    }
}

/*
 * Build MXP menu for items in inventory (in_room = FALSE)
 * Context-aware primary action based on item type
 * count = number of stacked items (for "Drop All" option)
 */
static void mxp_build_inventory_menu(OBJ_DATA *obj, CHAR_DATA *ch,
                                      const char *keywords, const char *escaped_hint,
                                      char *href_buf, size_t href_size,
                                      char *hint_buf, size_t hint_size,
                                      int count)
{
    const char *use_cmd = NULL;
    const char *use_label = NULL;
    OBJ_DATA *cont;
    char cont_keywords[MAX_INPUT_LENGTH];

    if (obj == NULL || keywords == NULL)
        return;

    href_buf[0] = '\0';
    hint_buf[0] = '\0';

    /* Determine primary use command based on item type */
    switch (obj->item_type)
    {
        case ITEM_POTION:
            use_cmd = "quaff"; use_label = "Quaff"; break;
        case ITEM_PILL:
        case ITEM_FOOD:
        case ITEM_EGG:
        case ITEM_QUEST:
        case ITEM_DTOKEN:
        case ITEM_DRAGONGEM:
            use_cmd = "eat"; use_label = "Eat"; break;
        case ITEM_DRINK_CON:
            use_cmd = "drink"; use_label = "Drink"; break;
        case ITEM_SCROLL:
            use_cmd = "recite"; use_label = "Recite"; break;
        case ITEM_WAND:
        case ITEM_STAFF:
            use_cmd = "brandish"; use_label = "Use"; break;
        case ITEM_BOOK:
            use_cmd = "open"; use_label = "Open"; break;
        case ITEM_PAGE:
            use_cmd = "read"; use_label = "Read"; break;
        case ITEM_INSTRUMENT:
            use_cmd = "play"; use_label = "Play"; break;
        case ITEM_STAKE:
        case ITEM_VOODOO:
        case ITEM_LIGHT:
            use_cmd = "hold"; use_label = "Hold"; break;
        case ITEM_HEAD:
            use_cmd = "sacrifice"; use_label = "Sacrifice"; break;
        case ITEM_COPPER:
        case ITEM_IRON:
        case ITEM_STEEL:
        case ITEM_ADAMANTITE:
        case ITEM_GEMSTONE:
        case ITEM_HILT:
            /* Forging materials - handle specially below */
            use_cmd = NULL; use_label = NULL; break;
        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            use_cmd = "look in"; use_label = "Look In"; break;
        default:
            /* Check wear flags for equipment */
            if (CAN_WEAR(obj, ITEM_WIELD))
            { use_cmd = "wield"; use_label = "Wield"; }
            else if (CAN_WEAR(obj, ITEM_HOLD))
            { use_cmd = "hold"; use_label = "Hold"; }
            else if (obj->wear_flags > ITEM_TAKE)
            { use_cmd = "wear"; use_label = "Wear"; }
            break;
    }

    /* Build the menu - start with primary action */
    if (use_cmd != NULL)
    {
        snprintf(href_buf, href_size, "%s '%s'", use_cmd, keywords);
        snprintf(hint_buf, hint_size, "%s|%s", escaped_hint, use_label);
    }
    else
    {
        strcpy(hint_buf, escaped_hint);
    }

    /* Add look */
    if (href_buf[0] != '\0')
    {
        strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
        strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
    }
    else
    {
        /* No primary action - hint already has tooltip, need separator before Look */
        strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
    }
    snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf), "look '%s'", keywords);
    strncat(hint_buf, "Look", hint_size - strlen(hint_buf) - 1);

    /* Add drop */
    strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
    strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
    snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf), "drop '%s'", keywords);
    strncat(hint_buf, "Drop", hint_size - strlen(hint_buf) - 1);

    /* Add drop all for stacked items */
    if (count > 1)
    {
        strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
        strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
        snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf), "drop all.'%s'", keywords);
        strncat(hint_buf, "Drop All", hint_size - strlen(hint_buf) - 1);
    }

    /* Add identify */
    strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
    strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
    snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf), "cast 'identify' '%s'", keywords);
    strncat(hint_buf, "Identify", hint_size - strlen(hint_buf) - 1);

    /* Add "Put in <container>" for each container in player's inventory (limit to 3) */
    if (obj->item_type != ITEM_CONTAINER && obj->item_type != ITEM_CORPSE_NPC
        && obj->item_type != ITEM_CORPSE_PC && ch->carrying != NULL)
    {
        int container_count = 0;
        const int max_containers = 3;

        for (cont = ch->carrying; cont != NULL && container_count < max_containers; cont = cont->next_content)
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
            strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
            strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
            snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf),
                "put '%s' '%s'", keywords, cont_keywords);
            snprintf(hint_buf + strlen(hint_buf), hint_size - strlen(hint_buf),
                "Put in %s", cont->short_descr);
            container_count++;
        }
    }

    /* Add "Forge onto <item>" for forging materials (limit to 5 targets to avoid overflow) */
    {
        bool is_metal = (obj->item_type == ITEM_COPPER || obj->item_type == ITEM_IRON
                      || obj->item_type == ITEM_STEEL || obj->item_type == ITEM_ADAMANTITE);
        bool is_gem = (obj->item_type == ITEM_GEMSTONE);
        bool is_hilt = (obj->item_type == ITEM_HILT);

        if ((is_metal || is_gem || is_hilt) && ch->carrying != NULL)
        {
            char target_keywords[MAX_INPUT_LENGTH];
            OBJ_DATA *count_obj;
            int item_index;
            int forge_count = 0;
            const int max_forge_targets = 5;

            for (cont = ch->carrying; cont != NULL && forge_count < max_forge_targets; cont = cont->next_content)
            {
                /* Skip the forging material itself */
                if (cont == obj)
                    continue;

                /* Skip worn items - can only forge items in inventory */
                if (cont->wear_loc != WEAR_NONE)
                    continue;

                /* Hilts can only be forged onto weapons */
                if (is_hilt && cont->item_type != ITEM_WEAPON)
                    continue;

                /* Metals and gems can be forged onto weapons and armor */
                if (!is_hilt && cont->item_type != ITEM_WEAPON && cont->item_type != ITEM_ARMOR)
                    continue;

                /* Skip items already forged with metal (for metal slabs) */
                if (is_metal && (IS_SET(cont->spectype, SITEM_COPPER)
                              || IS_SET(cont->spectype, SITEM_IRON)
                              || IS_SET(cont->spectype, SITEM_STEEL)
                              || IS_SET(cont->spectype, SITEM_ADAMANTITE)))
                    continue;

                /* Skip items already forged with gem (for gemstones) */
                if (is_gem && IS_SET(cont->spectype, SITEM_GEMSTONE))
                    continue;

                /* Get item name for command */
                if (cont->name == NULL || cont->name[0] == '\0')
                    continue;
                mxp_escape_string(target_keywords, cont->name, sizeof(target_keywords));

                /* Count how many items with same vnum come before this one */
                item_index = 1;
                for (count_obj = ch->carrying; count_obj != cont; count_obj = count_obj->next_content)
                {
                    if (count_obj->pIndexData->vnum == cont->pIndexData->vnum
                        && count_obj->wear_loc == WEAR_NONE)
                        item_index++;
                }

                /* Add to menu with index prefix for disambiguation */
                strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
                strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
                snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf),
                    "forge '%s' %d.'%s'", keywords, item_index, target_keywords);
                snprintf(hint_buf + strlen(hint_buf), hint_size - strlen(hint_buf),
                    "Forge onto %s", cont->short_descr);
                forge_count++;
            }
        }
    }
}

/*
 * Add SITEM special flag options to the menu
 * For items with SITEM_ACTIVATE, SITEM_PRESS, SITEM_TWIST, SITEM_PULL flags
 */
static void mxp_add_sitem_options(OBJ_DATA *obj, const char *keywords,
                                   char *href_buf, size_t href_size,
                                   char *hint_buf, size_t hint_size)
{
    if (obj == NULL || keywords == NULL)
        return;

    if (IS_SET(obj->spectype, SITEM_ACTIVATE))
    {
        strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
        strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
        snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf),
            "activate '%s'", keywords);
        strncat(hint_buf, "Activate", hint_size - strlen(hint_buf) - 1);
    }
    if (IS_SET(obj->spectype, SITEM_PRESS))
    {
        strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
        strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
        snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf),
            "press '%s'", keywords);
        strncat(hint_buf, "Press", hint_size - strlen(hint_buf) - 1);
    }
    if (IS_SET(obj->spectype, SITEM_TWIST))
    {
        strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
        strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
        snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf),
            "twist '%s'", keywords);
        strncat(hint_buf, "Twist", hint_size - strlen(hint_buf) - 1);
    }
    if (IS_SET(obj->spectype, SITEM_PULL))
    {
        strncat(href_buf, "|", href_size - strlen(href_buf) - 1);
        strncat(hint_buf, "|", hint_size - strlen(hint_buf) - 1);
        snprintf(href_buf + strlen(href_buf), href_size - strlen(href_buf),
            "pull '%s'", keywords);
        strncat(hint_buf, "Pull", hint_size - strlen(hint_buf) - 1);
    }
}

/*
 * Format an object with MXP clickable link and tooltip
 * Returns a static buffer with MXP-wrapped text
 *
 * in_room = TRUE:  Item is on the ground - context-aware primary action
 * in_room = FALSE: Item is in inventory - context-aware use action
 * count = number of stacked items (for "Drop All" option when count > 1)
 *
 * Tooltip shows item type, weight, material, condition, and item-specific info.
 */
char *mxp_obj_link(OBJ_DATA *obj, CHAR_DATA *ch, char *display_text, bool in_room, int count)
{
    static char buf[MXP_BUF_MAX_LEN];
    char keywords[MAX_INPUT_LENGTH];
    char hint[512];
    char escaped_hint[1024];
    char href_buf[2048];
    char hint_buf[2048];

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

    /* Build enhanced tooltip with item-specific information */
    mxp_build_item_tooltip(obj, ch, hint, sizeof(hint));
    mxp_escape_string(escaped_hint, hint, sizeof(escaped_hint));

    /* Build context-aware menu based on location */
    if (in_room)
    {
        mxp_build_ground_menu(obj, ch, keywords, escaped_hint,
            href_buf, sizeof(href_buf), hint_buf, sizeof(hint_buf));
    }
    else
    {
        mxp_build_inventory_menu(obj, ch, keywords, escaped_hint,
            href_buf, sizeof(href_buf), hint_buf, sizeof(hint_buf), count);
    }

    /* Add SITEM special flag options (activate/press/twist/pull) */
    mxp_add_sitem_options(obj, keywords, href_buf, sizeof(href_buf),
        hint_buf, sizeof(hint_buf));

    /* Build final MXP tag - href first, then hint for popup menu */
    snprintf(buf, sizeof(buf),
        MXP_SECURE_LINE "<SEND href=\"%s\" hint=\"%s\">%s</SEND>" MXP_LOCK_LOCKED,
        href_buf, hint_buf, display_text);

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

    /* Build the MXP-wrapped string: remove/look (and look in for containers)
     * Use single quotes around keywords to handle multi-word names like 'newbie sword' */
    {
        bool is_container = (obj->item_type == ITEM_CONTAINER
                          || obj->item_type == ITEM_CORPSE_NPC
                          || obj->item_type == ITEM_CORPSE_PC);

        if (is_container)
        {
            snprintf(buf, sizeof(buf),
                MXP_SECURE_LINE "<SEND href=\"remove '%s'|look '%s'|look in '%s'\" hint=\"%s|Remove|Look|Look In\">%s</SEND>" MXP_LOCK_LOCKED,
                keywords, keywords, keywords, escaped_hint, display_text);
        }
        else
        {
            snprintf(buf, sizeof(buf),
                MXP_SECURE_LINE "<SEND href=\"remove '%s'|look '%s'\" hint=\"%s|Remove|Look\">%s</SEND>" MXP_LOCK_LOCKED,
                keywords, keywords, escaped_hint, display_text);
        }
    }

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
    /* Use rotating buffers since exits are processed in a loop */
    static char buf[8][MXP_BUF_MAX_LEN];
    static int buf_index = 0;
    char *outbuf;
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

    /* Select next rotating buffer */
    outbuf = buf[buf_index];
    buf_index = (buf_index + 1) % 8;

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

    /* Build the MXP hint - room name plus occupant list */
    {
        char simple_hint[256];
        char clean_name[128];
        CHAR_DATA *vch;
        int count = 0;
        size_t len;

        if (IS_SET(pexit->exit_info, EX_CLOSED))
        {
            if (IS_SET(pexit->exit_info, EX_LOCKED))
                snprintf(simple_hint, sizeof(simple_hint), "Locked door");
            else
                snprintf(simple_hint, sizeof(simple_hint), "Closed door");
        }
        else if (room_is_dark(to_room) && !IS_AFFECTED(ch, AFF_INFRARED)
                 && !IS_SET(ch->act, PLR_HOLYLIGHT))
        {
            snprintf(simple_hint, sizeof(simple_hint), "Too dark to see");
        }
        else
        {
            /* Start with room name (color stripped) */
            mxp_strip_colors(clean_name, to_room->name ? to_room->name : "Unknown",
                sizeof(clean_name));
            snprintf(simple_hint, sizeof(simple_hint), "%s", clean_name);
            len = strlen(simple_hint);

            /* Add occupants - players first, then NPCs */
            for (vch = to_room->people; vch != NULL; vch = vch->next_in_room)
            {
                char name_buf[64];
                const char *name;

                if (IS_NPC(vch))
                    continue;  /* Skip NPCs in first pass */
                if (!can_see(ch, vch))
                    continue;

                if (IS_AFFECTED(vch, AFF_POLYMORPH) && vch->morph != NULL)
                    name = vch->morph;
                else
                    name = vch->name;

                mxp_strip_colors(name_buf, name, sizeof(name_buf));

                if (count == 0)
                    len += snprintf(simple_hint + len, sizeof(simple_hint) - len, " - %s", name_buf);
                else if (count < 4)
                    len += snprintf(simple_hint + len, sizeof(simple_hint) - len, ", %s", name_buf);
                else
                {
                    len += snprintf(simple_hint + len, sizeof(simple_hint) - len, "...");
                    break;
                }
                count++;
            }

            /* Second pass: NPCs */
            if (count <= 4)
            {
                for (vch = to_room->people; vch != NULL; vch = vch->next_in_room)
                {
                    char name_buf[64];

                    if (!IS_NPC(vch))
                        continue;
                    if (!can_see(ch, vch))
                        continue;

                    mxp_strip_colors(name_buf, vch->short_descr, sizeof(name_buf));

                    if (count == 0)
                        len += snprintf(simple_hint + len, sizeof(simple_hint) - len, " - %s", name_buf);
                    else if (count < 4)
                        len += snprintf(simple_hint + len, sizeof(simple_hint) - len, ", %s", name_buf);
                    else
                    {
                        len += snprintf(simple_hint + len, sizeof(simple_hint) - len, "...");
                        break;
                    }
                    count++;
                }
            }
        }
        mxp_escape_string(escaped_hint, simple_hint, sizeof(escaped_hint));
    }

    snprintf(outbuf, MXP_BUF_MAX_LEN,
        "<SEND href=\"%s\" hint=\"%s\">%s</SEND>",
        cmd, escaped_hint, display_text);

    return outbuf;
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

    /* Get keyword to target this character - use full name with quotes
     * to reduce mismatches when multiple similar NPCs are in room */
    if (IS_NPC(victim))
    {
        /* Use full NPC name in quotes for precise targeting */
        if (victim->name == NULL || victim->name[0] == '\0')
            return display_text;
        snprintf(keyword, sizeof(keyword), "'%s'", victim->name);
    }
    else
    {
        /* Use player name */
        if (IS_AFFECTED(victim, AFF_POLYMORPH) && victim->morph != NULL)
        {
            /* Polymorphed - use full morph name in quotes for targeting */
            if (victim->morph[0] != '\0')
                snprintf(keyword, sizeof(keyword), "'%s'", victim->morph);
            else
                snprintf(keyword, sizeof(keyword), "%s", victim->name);
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
