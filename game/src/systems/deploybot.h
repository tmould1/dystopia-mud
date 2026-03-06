#ifndef DEPLOYBOT_H
#define DEPLOYBOT_H

#if !defined( WIN32 )

/* In-game deployment pipeline — Linux only.
 *
 * Provides the 'deploybot' command (level 12) which runs server update
 * scripts (download/validate/migrate) asynchronously via fork(), posting
 * results to the immtalk channel as "DeployBot".
 *
 * Scripts are expected at {mud_base_dir}/../../scripts/ which maps to
 * server/scripts/ when the binary runs from server/live/gamedata/.
 */

void do_deploybot( CHAR_DATA *ch, char *argument );
void deploybot_check( void ); /* call each pulse from game_tick() */

#endif /* !WIN32 */

#endif /* DEPLOYBOT_H */
