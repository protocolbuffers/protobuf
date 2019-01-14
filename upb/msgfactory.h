
#include "upb/def.h"
#include "upb/msg.h"

#ifndef UPB_MSGFACTORY_H_
#define UPB_MSGFACTORY_H_

/** upb_msgfactory ************************************************************/

struct upb_msgfactory;
typedef struct upb_msgfactory upb_msgfactory;

#ifdef __cplusplus
extern "C" {
#endif

/* A upb_msgfactory contains a cache of upb_msglayout, upb_handlers, and
 * upb_visitorplan objects.  These are the objects necessary to represent,
 * populate, and and visit upb_msg objects.
 *
 * These caches are all populated by upb_msgdef, and lazily created on demand.
 */

/* Creates and destroys a msgfactory, respectively.  The messages for this
 * msgfactory must come from |symtab| (which should outlive the msgfactory). */
upb_msgfactory *upb_msgfactory_new(const upb_symtab *symtab);
void upb_msgfactory_free(upb_msgfactory *f);

const upb_symtab *upb_msgfactory_symtab(const upb_msgfactory *f);

/* The functions to get cached objects, lazily creating them on demand.  These
 * all require:
 *
 * - m is in upb_msgfactory_symtab(f)
 * - upb_msgdef_mapentry(m) == false (since map messages can't have layouts).
 *
 * The returned objects will live for as long as the msgfactory does.
 *
 * TODO(haberman): consider making this thread-safe and take a const
 * upb_msgfactory. */
const upb_msglayout *upb_msgfactory_getlayout(upb_msgfactory *f,
                                              const upb_msgdef *m);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* UPB_MSGFACTORY_H_ */
