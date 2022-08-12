/* Codebase macros - Change as you might need.
 * Yes, Rogel, you can gloat all you want. You win, this is cleaner, though not by a whole lot.
 */

#ifndef __IMC2CFG_H__
#define __IMC2CFG_H__

#define SMAUGSOCIAL

extern struct social_messg *soc_mess_list;
struct social_messg *find_social( const char *name );

typedef struct social_messg SOCIAL_DATA;
typedef struct char_data CHAR_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;

extern const char *class_abbrevs[];

const char *title_female( int chclass, int level );
const char *title_male( int chclass, int level );

#define first_descriptor         descriptor_list
#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define CH_IMCDATA(ch)           ((ch)->player_specials->imcchardata)
#define CH_IMCLEVEL(ch)          GET_ADMLEVEL(ch)
#define CH_IMCNAME(ch)           GET_NAME(ch)
#define CH_IMCTITLE(ch)          GET_TITLE(ch)
#define CH_IMCRANK(ch)           (GET_SEX(ch) == SEX_FEMALE ? "Female" : "Male")
#define CH_IMCSEX(ch)            GET_SEX(ch)

#endif

