
#ifndef __foo_marshal_MARSHAL_H__
#define __foo_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:OBJECT,OBJECT (./foo-marshal.list:1) */
extern void foo_marshal_VOID__OBJECT_OBJECT (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* VOID:UINT,UINT,UINT,UINT (./foo-marshal.list:2) */
extern void foo_marshal_VOID__UINT_UINT_UINT_UINT (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:UINT,UINT (./foo-marshal.list:3) */
extern void foo_marshal_VOID__UINT_UINT (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

/* VOID:BOXED,BOXED (./foo-marshal.list:4) */
extern void foo_marshal_VOID__BOXED_BOXED (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* VOID:POINTER,BOXED,POINTER (./foo-marshal.list:5) */
extern void foo_marshal_VOID__POINTER_BOXED_POINTER (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);

/* VOID:POINTER,POINTER (./foo-marshal.list:6) */
extern void foo_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

G_END_DECLS

#endif /* __foo_marshal_MARSHAL_H__ */

