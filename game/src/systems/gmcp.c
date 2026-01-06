/*
 * gmcp.c - Generic MUD Communication Protocol support
 *
 * GMCP enables structured JSON data exchange with modern MUD clients.
 * Uses telnet option 201 (TELOPT_GMCP) with subnegotiation.
 *
 * Protocol: IAC SB GMCP <package.name> <JSON data> IAC SE
 *
 * Supported packages:
 *   Core.Hello, Core.Goodbye, Core.Supports.Set
 *   Char.Vitals, Char.Status, Char.Info
 */

#include "merc.h"
#if !defined(WIN32)
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "telnet.h"
#include "gmcp.h"
#include "class.h"

/*
 * Telnet negotiation strings for GMCP
 */
#if defined(WIN32)
const char gmcp_will[] = { (char)IAC, (char)WILL, (char)TELOPT_GMCP, '\0' };
const char gmcp_wont[] = { (char)IAC, (char)WONT, (char)TELOPT_GMCP, '\0' };
const char gmcp_do[]   = { (char)IAC, (char)DO, (char)TELOPT_GMCP, '\0' };
const char gmcp_dont[] = { (char)IAC, (char)DONT, (char)TELOPT_GMCP, '\0' };
#else
const char gmcp_will[] = { IAC, WILL, TELOPT_GMCP, '\0' };
const char gmcp_wont[] = { IAC, WONT, TELOPT_GMCP, '\0' };
const char gmcp_do[]   = { IAC, DO, TELOPT_GMCP, '\0' };
const char gmcp_dont[] = { IAC, DONT, TELOPT_GMCP, '\0' };
#endif

/* External functions */
bool write_to_descriptor args( ( DESCRIPTOR_DATA *d, char *txt, int length ) );

/* External data */
extern GAMECONFIG_DATA game_config;

/*
 * Get class name from class bitmask
 */
static const char *get_class_name(int class_bits)
{
    if (class_bits & CLASS_DEMON)         return "Demon";
    if (class_bits & CLASS_MAGE)          return "Mage";
    if (class_bits & CLASS_WEREWOLF)      return "Werewolf";
    if (class_bits & CLASS_VAMPIRE)       return "Vampire";
    if (class_bits & CLASS_SAMURAI)       return "Samurai";
    if (class_bits & CLASS_DROW)          return "Drow";
    if (class_bits & CLASS_MONK)          return "Monk";
    if (class_bits & CLASS_NINJA)         return "Ninja";
    if (class_bits & CLASS_LICH)          return "Lich";
    if (class_bits & CLASS_SHAPESHIFTER)  return "Shapeshifter";
    if (class_bits & CLASS_TANARRI)       return "Tanarri";
    if (class_bits & CLASS_ANGEL)         return "Angel";
    if (class_bits & CLASS_UNDEAD_KNIGHT) return "Undead Knight";
    if (class_bits & CLASS_DROID)         return "Droid";
    return "None";
}

/*
 * Get position name
 */
static const char *get_pos_name(int position)
{
    switch (position)
    {
        case POS_DEAD:      return "dead";
        case POS_MORTAL:    return "mortally wounded";
        case POS_INCAP:     return "incapacitated";
        case POS_STUNNED:   return "stunned";
        case POS_SLEEPING:  return "sleeping";
        case POS_MEDITATING:return "meditating";
        case POS_SITTING:   return "sitting";
        case POS_RESTING:   return "resting";
        case POS_FIGHTING:  return "fighting";
        case POS_STANDING:  return "standing";
        default:            return "unknown";
    }
}

/*
 * Escape a string for JSON output
 * Returns a static buffer - not reentrant
 */
static char *json_escape(const char *str)
{
    static char buf[MAX_STRING_LENGTH];
    char *ptr = buf;

    if (str == NULL)
    {
        buf[0] = '\0';
        return buf;
    }

    while (*str && ptr < buf + sizeof(buf) - 6)
    {
        switch (*str)
        {
            case '"':   *ptr++ = '\\'; *ptr++ = '"';  break;
            case '\\':  *ptr++ = '\\'; *ptr++ = '\\'; break;
            case '\n':  *ptr++ = '\\'; *ptr++ = 'n';  break;
            case '\r':  *ptr++ = '\\'; *ptr++ = 'r';  break;
            case '\t':  *ptr++ = '\\'; *ptr++ = 't';  break;
            default:
                if ((unsigned char)*str >= 32)
                    *ptr++ = *str;
                break;
        }
        str++;
    }
    *ptr = '\0';
    return buf;
}

/*
 * Initialize GMCP on a descriptor after client sends IAC DO GMCP
 */
void gmcp_init(DESCRIPTOR_DATA *d)
{
    if (d == NULL)
        return;

    d->gmcp_enabled = TRUE;

    /* Enable Core and all Char packages by default */
    d->gmcp_packages = GMCP_PACKAGE_CORE | GMCP_PACKAGE_CHAR |
                       GMCP_PACKAGE_CHAR_VITALS | GMCP_PACKAGE_CHAR_STATUS |
                       GMCP_PACKAGE_CHAR_INFO;

    /* Send Core.Hello to identify the server */
    gmcp_send(d, "Core.Hello",
        "{\"name\":\"Dystopia MUD\",\"version\":\"1.0\"}");
}

/*
 * Send a raw GMCP message: IAC SB GMCP <package> <data> IAC SE
 */
void gmcp_send(DESCRIPTOR_DATA *d, const char *package, const char *data)
{
    char buf[MAX_STRING_LENGTH * 2];
    int len;

    if (d == NULL || !d->gmcp_enabled)
        return;

    if (package == NULL)
        return;

    /* Build the GMCP message */
    buf[0] = (char)IAC;
    buf[1] = (char)SB;
    buf[2] = (char)TELOPT_GMCP;
    len = 3;

    /* Copy package name */
    while (*package && len < sizeof(buf) - 10)
        buf[len++] = *package++;

    /* Add space separator if we have data */
    if (data != NULL && *data)
    {
        buf[len++] = ' ';

        /* Copy JSON data */
        while (*data && len < sizeof(buf) - 5)
            buf[len++] = *data++;
    }

    /* End subnegotiation */
    buf[len++] = (char)IAC;
    buf[len++] = (char)SE;
    buf[len] = '\0';

    write_to_descriptor(d, buf, len);
}

/*
 * Send Char.Vitals with current/max hp, mana, move
 */
void gmcp_send_vitals(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];

    if (ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled)
        return;

    /* Check if client supports Char.Vitals */
    if (!(ch->desc->gmcp_packages & (GMCP_PACKAGE_CHAR | GMCP_PACKAGE_CHAR_VITALS)))
        return;

    snprintf(buf, sizeof(buf),
        "{\"hp\":%d,\"maxhp\":%d,\"mana\":%d,\"maxmana\":%d,\"move\":%d,\"maxmove\":%d}",
        ch->hit, ch->max_hit,
        ch->mana, ch->max_mana,
        ch->move, ch->max_move);

    gmcp_send(ch->desc, "Char.Vitals", buf);
}

/*
 * Send Char.Status with level, class, position, experience
 */
void gmcp_send_status(CHAR_DATA *ch)
{
    char buf[512];
    char class_buf[64];
    char pos_buf[64];

    if (ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled)
        return;

    /* Check if client supports Char.Status */
    if (!(ch->desc->gmcp_packages & (GMCP_PACKAGE_CHAR | GMCP_PACKAGE_CHAR_STATUS)))
        return;

    /* Copy escaped strings to avoid static buffer issues */
    strncpy(class_buf, json_escape(get_class_name(ch->class)), sizeof(class_buf) - 1);
    class_buf[sizeof(class_buf) - 1] = '\0';
    strncpy(pos_buf, json_escape(get_pos_name(ch->position)), sizeof(pos_buf) - 1);
    pos_buf[sizeof(pos_buf) - 1] = '\0';

    snprintf(buf, sizeof(buf),
        "{\"level\":%d,\"class\":\"%s\",\"position\":\"%s\",\"exp\":%d}",
        ch->level, class_buf, pos_buf, ch->exp);

    gmcp_send(ch->desc, "Char.Status", buf);
}

/*
 * Send Char.Info with name, race, guild/clan
 */
void gmcp_send_info(CHAR_DATA *ch)
{
    char buf[512];
    char name_buf[128];
    char clan_buf[128];
    const char *clan_name;

    if (ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled)
        return;

    /* Check if client supports Char.Info */
    if (!(ch->desc->gmcp_packages & (GMCP_PACKAGE_CHAR | GMCP_PACKAGE_CHAR_INFO)))
        return;

    clan_name = (ch->clan != NULL && ch->clan[0] != '\0') ? ch->clan : "None";

    /* Copy escaped strings to avoid static buffer issues */
    strncpy(name_buf, json_escape(ch->name), sizeof(name_buf) - 1);
    name_buf[sizeof(name_buf) - 1] = '\0';
    strncpy(clan_buf, json_escape(clan_name), sizeof(clan_buf) - 1);
    clan_buf[sizeof(clan_buf) - 1] = '\0';

    snprintf(buf, sizeof(buf),
        "{\"name\":\"%s\",\"guild\":\"%s\"}",
        name_buf, clan_buf);

    gmcp_send(ch->desc, "Char.Info", buf);
}

/*
 * Send Client.GUI for Mudlet auto-install package
 * Sends the URL and version of a Mudlet mpackage that clients will
 * automatically download and install.
 */
void gmcp_send_gui(DESCRIPTOR_DATA *d)
{
    char buf[MAX_STRING_LENGTH];

    if (d == NULL || !d->gmcp_enabled)
        return;

    /* Skip if no GUI URL configured */
    if (game_config.gui_url == NULL || game_config.gui_url[0] == '\0')
        return;

    snprintf(buf, sizeof(buf),
        "{\"version\":\"%s\",\"url\":\"%s\"}",
        game_config.gui_version,
        game_config.gui_url);

    gmcp_send(d, "Client.GUI", buf);
}

/*
 * Send all character data (called on login)
 */
void gmcp_send_char_data(CHAR_DATA *ch)
{
    if (ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled)
        return;

    gmcp_send_gui(ch->desc);  /* Send GUI package first */
    gmcp_send_info(ch);
    gmcp_send_status(ch);
    gmcp_send_vitals(ch);
}

/*
 * Parse a package name and version from Core.Supports.Set
 * Format: "Package.Name version" or just "Package.Name"
 * Returns the package flag if recognized, 0 otherwise
 */
static int parse_package_support(const char *pkg)
{
    if (pkg == NULL)
        return 0;

    /* Skip leading whitespace and quotes */
    while (*pkg == ' ' || *pkg == '"' || *pkg == '[')
        pkg++;

    /* Check for known packages */
    if (!strncmp(pkg, "Char.Vitals", 11))
        return GMCP_PACKAGE_CHAR_VITALS | GMCP_PACKAGE_CHAR;
    if (!strncmp(pkg, "Char.Status", 11))
        return GMCP_PACKAGE_CHAR_STATUS | GMCP_PACKAGE_CHAR;
    if (!strncmp(pkg, "Char.Info", 9))
        return GMCP_PACKAGE_CHAR_INFO | GMCP_PACKAGE_CHAR;
    if (!strncmp(pkg, "Char", 4))
        return GMCP_PACKAGE_CHAR | GMCP_PACKAGE_CHAR_VITALS |
               GMCP_PACKAGE_CHAR_STATUS | GMCP_PACKAGE_CHAR_INFO;
    if (!strncmp(pkg, "Core", 4))
        return GMCP_PACKAGE_CORE;

    return 0;
}

/*
 * Handle incoming GMCP subnegotiation from client
 * Data format: <package.name> <JSON data>
 */
void gmcp_handle_subnegotiation(DESCRIPTOR_DATA *d, unsigned char *data, int len)
{
    char package[256];
    char json_data[MAX_STRING_LENGTH];
    int i, j;

    if (d == NULL || data == NULL || len <= 0)
        return;

    /* Extract package name (up to first space or end) */
    for (i = 0; i < len && i < (int)sizeof(package) - 1 && data[i] != ' '; i++)
        package[i] = data[i];
    package[i] = '\0';

    /* Skip the space */
    if (i < len && data[i] == ' ')
        i++;

    /* Extract JSON data (rest of the message) */
    for (j = 0; i < len && j < (int)sizeof(json_data) - 1; i++, j++)
        json_data[j] = data[i];
    json_data[j] = '\0';

    /* Handle Core.Supports.Set - client tells us what packages it wants */
    if (!strcmp(package, "Core.Supports.Set"))
    {
        /* Parse the JSON array of supported packages */
        /* Format: ["Char 1", "Char.Vitals 1", ...] */
        char *ptr = json_data;
        char pkg_buf[128];
        int pkg_i;

        while (*ptr)
        {
            /* Find start of package name (after quote) */
            while (*ptr && *ptr != '"')
                ptr++;
            if (*ptr == '"')
                ptr++;

            /* Extract package name */
            pkg_i = 0;
            while (*ptr && *ptr != '"' && pkg_i < (int)sizeof(pkg_buf) - 1)
                pkg_buf[pkg_i++] = *ptr++;
            pkg_buf[pkg_i] = '\0';

            /* Parse and add to supported packages */
            if (pkg_i > 0)
                d->gmcp_packages |= parse_package_support(pkg_buf);

            /* Skip to next */
            if (*ptr == '"')
                ptr++;
        }
    }
    /* Handle Core.Supports.Add - add a package */
    else if (!strcmp(package, "Core.Supports.Add"))
    {
        d->gmcp_packages |= parse_package_support(json_data);
    }
    /* Handle Core.Supports.Remove - remove a package */
    else if (!strcmp(package, "Core.Supports.Remove"))
    {
        d->gmcp_packages &= ~parse_package_support(json_data);
    }
    /* Core.Hello - client identification (we just log/ignore) */
    else if (!strcmp(package, "Core.Hello"))
    {
        /* Could parse client name/version from json_data if needed */
    }
}

/*
 * User command to show GMCP status
 */
void do_gmcp(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (ch->desc == NULL)
    {
        send_to_char("No descriptor.\n\r", ch);
        return;
    }

    if (!ch->desc->gmcp_enabled)
    {
        send_to_char("GMCP is not enabled. Your client does not support it.\n\r", ch);
        return;
    }

    snprintf(buf, sizeof(buf),
        "GMCP Status:\n\r"
        "  Enabled: Yes\n\r"
        "  Packages: %s%s%s%s%s\n\r",
        (ch->desc->gmcp_packages & GMCP_PACKAGE_CORE) ? "Core " : "",
        (ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR) ? "Char " : "",
        (ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_VITALS) ? "Char.Vitals " : "",
        (ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_STATUS) ? "Char.Status " : "",
        (ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_INFO) ? "Char.Info " : "");

    send_to_char(buf, ch);
}
