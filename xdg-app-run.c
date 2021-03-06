#include "config.h"

#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <gio/gio.h>
#include "libgsystem.h"

#include "xdg-app-run.h"
#include "xdg-app-utils.h"

gboolean
xdg_app_run_verify_environment_keys (const char **keys,
				     GError **error)
{
  const char *key;
  const char *environment_keys[] = {
    "x11", "wayland", "ipc", "pulseaudio", "system-dbus", "session-dbus",
    "network", "host-fs", "homedir", NULL
  };

  if (keys == NULL)
    return TRUE;

  if ((key = g_strv_subset (environment_keys, keys)) != NULL)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
		   "Unknown Environment key %s", key);
      return FALSE;
    }

  return TRUE;
}


static char *
extract_unix_path_from_dbus_address (const char *address)
{
  const char *path, *path_end;

  if (address == NULL)
    return NULL;

  if (!g_str_has_prefix (address, "unix:"))
    return NULL;

  path = strstr (address, "path=");
  if (path == NULL)
    return NULL;
  path += strlen ("path=");
  path_end = path;
  while (*path_end != 0 && *path_end != ',')
    path_end++;

  return g_strndup (path, path_end - path);
}

void
xdg_app_run_add_x11_args (GPtrArray *argv_array)
{
  char *x11_socket = NULL;
  const char *display = g_getenv ("DISPLAY");

  if (display && display[0] == ':' && g_ascii_isdigit (display[1]))
    {
      const char *display_nr = &display[1];
      const char *display_nr_end = display_nr;
      gs_free char *d = NULL;

      while (g_ascii_isdigit (*display_nr_end))
        display_nr_end++;

      d = g_strndup (display_nr, display_nr_end - display_nr);
      x11_socket = g_strdup_printf ("/tmp/.X11-unix/X%s", d);

      g_ptr_array_add (argv_array, g_strdup ("-x"));
      g_ptr_array_add (argv_array, x11_socket);
    }
}

void
xdg_app_run_add_wayland_args (GPtrArray *argv_array)
{
  char *wayland_socket = g_build_filename (g_get_user_runtime_dir (), "wayland-0", NULL);
  if (g_file_test (wayland_socket, G_FILE_TEST_EXISTS))
    {
      g_ptr_array_add (argv_array, g_strdup ("-y"));
      g_ptr_array_add (argv_array, wayland_socket);
    }
  else
    g_free (wayland_socket);
}

void
xdg_app_run_add_pulseaudio_args (GPtrArray *argv_array)
{
  char *pulseaudio_socket = g_build_filename (g_get_user_runtime_dir (), "pulse/native", NULL);
  if (g_file_test (pulseaudio_socket, G_FILE_TEST_EXISTS))
    {
      g_ptr_array_add (argv_array, g_strdup ("-p"));
      g_ptr_array_add (argv_array, pulseaudio_socket);
    }
}

void
xdg_app_run_add_system_dbus_args (GPtrArray *argv_array)
{
  const char *dbus_address = g_getenv ("DBUS_SYSTEM_BUS_ADDRESS");
  char *dbus_system_socket = NULL;

  dbus_system_socket = extract_unix_path_from_dbus_address (dbus_address);
  if (dbus_system_socket == NULL &&
      g_file_test ("/var/run/dbus/system_bus_socket", G_FILE_TEST_EXISTS))
    {
      dbus_system_socket = g_strdup ("/var/run/dbus/system_bus_socket");
    }

  if (dbus_system_socket != NULL)
    {
      g_ptr_array_add (argv_array, g_strdup ("-D"));
      g_ptr_array_add (argv_array, dbus_system_socket);
    }
}

void
xdg_app_run_add_session_dbus_args (GPtrArray *argv_array)
{
  const char *dbus_address = g_getenv ("DBUS_SESSION_BUS_ADDRESS");
  char *dbus_session_socket = NULL;

  dbus_session_socket = extract_unix_path_from_dbus_address (dbus_address);
  if (dbus_session_socket != NULL)
    {
      g_ptr_array_add (argv_array, g_strdup ("-d"));
      g_ptr_array_add (argv_array, dbus_session_socket);
    }
}

void
xdg_app_run_add_environment_args (GPtrArray *argv_array,
				  GKeyFile *metakey,
				  const char **allow,
				  const char **forbid)
{
  const char *no_opts[1] = { NULL };
  char opts[16];
  int i;

  i = 0;
  opts[i++] = '-';

  if (allow == NULL)
    allow = no_opts;

  if (forbid == NULL)
    forbid = no_opts;

  if ((g_key_file_get_boolean (metakey, "Environment", "ipc", NULL) || g_strv_contains (allow, "ipc")) &&
      !g_strv_contains (forbid, "ipc"))
    {
      g_debug ("Allowing ipc access");
      opts[i++] = 'i';
    }

  if ((g_key_file_get_boolean (metakey, "Environment", "host-fs", NULL) || g_strv_contains (allow, "nost-fs")) &&
      !g_strv_contains (forbid, "host-fs"))
    {
      g_debug ("Allowing host-fs access");
      opts[i++] = 'f';
    }

  if ((g_key_file_get_boolean (metakey, "Environment", "homedir", NULL) || g_strv_contains (allow, "homedir")) &&
      !g_strv_contains (forbid, "homedir"))
    {
      g_debug ("Allowing homedir access");
      opts[i++] = 'H';
    }

  if ((g_key_file_get_boolean (metakey, "Environment", "network", NULL) || g_strv_contains (allow, "network")) &&
      !g_strv_contains (forbid, "network"))
    {
      g_debug ("Allowing network access");
      opts[i++] = 'n';
    }

  if ((g_key_file_get_boolean (metakey, "Environment", "x11", NULL) || g_strv_contains (allow, "x11")) &&
      !g_strv_contains (forbid, "x11"))
    {
      g_debug ("Allowing x11 access");
      xdg_app_run_add_x11_args (argv_array);
    }

  if ((g_key_file_get_boolean (metakey, "Environment", "wayland", NULL) || g_strv_contains (allow, "wayland")) &&
      !g_strv_contains (forbid, "wayland"))
    {
      g_debug ("Allowing wayland access");
      xdg_app_run_add_wayland_args (argv_array);
    }

  if ((g_key_file_get_boolean (metakey, "Environment", "pulseaudio", NULL) || g_strv_contains (allow, "pulseaudio")) &&
      !g_strv_contains (forbid, "pulseaudio"))
    {
      g_debug ("Allowing pulseaudio access");
      xdg_app_run_add_pulseaudio_args (argv_array);
    }

  if ((g_key_file_get_boolean (metakey, "Environment", "system-dbus", NULL) || g_strv_contains (allow, "system-dbus")) &&
      !g_strv_contains (forbid, "system-dbus"))
    {
      g_debug ("Allowing system-dbus access");
      xdg_app_run_add_system_dbus_args (argv_array);
    }

  if ((g_key_file_get_boolean (metakey, "Environment", "session-dbus", NULL) || g_strv_contains (allow, "session-dbus")) &&
      !g_strv_contains (forbid, "session-dbus"))
    {
      g_debug ("Allowing session-dbus access");
      xdg_app_run_add_session_dbus_args (argv_array);
    }

  g_assert (sizeof(opts) > i);
  if (i > 1)
    {
      opts[i++] = 0;
      g_ptr_array_add (argv_array, g_strdup (opts));
    }
}

void
xdg_app_run_setup_minimal_env (GPtrArray *env_array,
			       gboolean devel)
{
  static const char const *exports[] = {
    "XDG_DATA_DIRS=/self/share:/usr/share",
    "PATH=/self/bin:/usr/bin",
    "SHELL=/bin/sh",
  };
  static const char const *exports_devel[] = {
    "ACLOCAL_PATH=/self/share/aclocal",
    "C_INCLUDE_PATH=/self/include",
    "CPLUS_INCLUDE_PATH=/self/include",
    "GI_TYPELIB_PATH=/self/lib/girepository-1.0",
    "LDFLAGS=-L/self/lib ",
    "PKG_CONFIG_PATH=/self/lib/pkgconfig:/self/share/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig",
    "LC_ALL=en_US.utf8",
  };
  static const char const *copy[] = {
    "GDMSESSION",
    "XDG_CURRENT_DESKTOP",
    "XDG_SESSION_DESKTOP",
    "DESKTOP_SESSION",
    "EMAIL_ADDRESS",
    "HOME",
    "HOSTNAME",
    "LOGNAME",
    "REAL_NAME",
    "TERM",
    "USER",
    "USERNAME",
  };
  static const char const *copy_nodevel[] = {
    "LANG",
    "LANGUAGE",
    "LC_ALL",
    "LC_ADDRESS",
    "LC_COLLATE",
    "LC_CTYPE",
    "LC_IDENTIFICATION",
    "LC_MEASUREMENT",
    "LC_MESSAGES",
    "LC_MONETARY",
    "LC_NAME",
    "LC_NUMERIC",
    "LC_PAPER",
    "LC_TELEPHONE",
    "LC_TIME",
  };
  int i;

  for (i = 0; i < G_N_ELEMENTS(exports); i++)
    g_ptr_array_add (env_array, g_strdup (exports[i]));

  if (devel)
    {
      for (i = 0; i < G_N_ELEMENTS(exports_devel); i++)
	g_ptr_array_add (env_array, g_strdup (exports_devel[i]));
    }

  for (i = 0; i < G_N_ELEMENTS(copy); i++)
    {
      const char *current = g_getenv(copy[i]);
      if (current)
	g_ptr_array_add (env_array, g_strdup_printf ("%s=%s", copy[i], current));
    }

  if (!devel)
    {
      for (i = 0; i < G_N_ELEMENTS(copy_nodevel); i++)
	{
	  const char *current = g_getenv(copy_nodevel[i]);
	  if (current)
	    g_ptr_array_add (env_array, g_strdup_printf ("%s=%s", copy_nodevel[i], current));
	}
    }
}

GFile *
xdg_app_get_data_dir (const char *app_id)
{
  gs_unref_object GFile *home = g_file_new_for_path (g_get_home_dir ());
  gs_unref_object GFile *var_app = g_file_resolve_relative_path (home, ".var/app");

  return g_file_get_child (var_app, app_id);
}

GFile *
xdg_app_ensure_data_dir (const char *app_id,
			 GCancellable  *cancellable,
			 GError **error)
{
  gs_unref_object GFile *dir = xdg_app_get_data_dir (app_id);
  gs_unref_object GFile *data_dir = g_file_get_child (dir, "data");
  gs_unref_object GFile *cache_dir = g_file_get_child (dir, "cache");
  gs_unref_object GFile *config_dir = g_file_get_child (dir, "config");

  if (!gs_file_ensure_directory (data_dir, TRUE, cancellable, error))
    return NULL;

  if (!gs_file_ensure_directory (cache_dir, TRUE, cancellable, error))
    return NULL;

  if (!gs_file_ensure_directory (config_dir, TRUE, cancellable, error))
    return NULL;

  return g_object_ref (dir);
}
