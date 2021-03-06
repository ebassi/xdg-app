#ifndef __XDG_APP_RUN_H__
#define __XDG_APP_RUN_H__

gboolean xdg_app_run_verify_environment_keys (const char **keys,
					      GError     **error);
void     xdg_app_run_add_environment_args    (GPtrArray   *argv_array,
					      GKeyFile    *metakey,
					      const char **allow,
					      const char **forbid);
void     xdg_app_run_setup_minimal_env       (GPtrArray   *env_array,
					      gboolean     devel);

void xdg_app_run_add_x11_args          (GPtrArray *argv_array);
void xdg_app_run_add_no_x11_args       (GPtrArray *argv_array);
void xdg_app_run_add_wayland_args      (GPtrArray *argv_array);
void xdg_app_run_add_pulseaudio_args   (GPtrArray *argv_array);
void xdg_app_run_add_system_dbus_args  (GPtrArray *argv_array);
void xdg_app_run_add_session_dbus_args (GPtrArray *argv_array);

GFile *xdg_app_get_data_dir (const char *app_id);
GFile *xdg_app_ensure_data_dir (const char *app_id,
				GCancellable  *cancellable,
				GError **error);

#endif /* __XDG_APP_RUN_H__ */
