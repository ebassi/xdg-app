#include "config.h"

#include <locale.h>
#include <stdlib.h>
#include <unistd.h>

#include "libgsystem.h"

#include "xdg-app-builtins.h"
#include "xdg-app-utils.h"

static gboolean opt_no_gpg_verify;
static gboolean opt_if_not_exists;
static char *opt_title;

static GOptionEntry options[] = {
  { "no-gpg-verify", 0, 0, G_OPTION_ARG_NONE, &opt_no_gpg_verify, "Disable GPG verification", NULL },
  { "if-not-exists", 0, 0, G_OPTION_ARG_NONE, &opt_if_not_exists, "Do nothing if the provided remote exists", NULL },
  { "title", 0, 0, G_OPTION_ARG_STRING, &opt_title, "A nice name to use for this remote", "TITLE" },
  { NULL }
};

gboolean
xdg_app_builtin_add_remote (int argc, char **argv, GCancellable *cancellable, GError **error)
{
  GOptionContext *context;
  gboolean ret = FALSE;
  gs_unref_object XdgAppDir *dir = NULL;
  gs_unref_variant_builder GVariantBuilder *optbuilder = NULL;
  gs_unref_hashtable GHashTable *refs = NULL;
  gs_free char *title = NULL;
  const char *remote_name;
  const char *remote_url;

  context = g_option_context_new ("NAME URL - Add a remote repository");

  if (!xdg_app_option_context_parse (context, options, &argc, &argv, 0, &dir, cancellable, error))
    goto out;

  if (argc < 3)
    {
      usage_error (context, "NAME and URL must be specified", error);
      goto out;
    }

  remote_name = argv[1];
  remote_url  = argv[2];

  optbuilder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));

  if (!ostree_repo_load_summary (remote_url, &refs, &title, cancellable, error))
    goto out;

  if (opt_no_gpg_verify)
    g_variant_builder_add (optbuilder, "{s@v}",
                           "gpg-verify",
                           g_variant_new_variant (g_variant_new_boolean (FALSE)));

  if (opt_title)
    {
      g_free (title);
      title = g_strdup (opt_title);
    }

  if (title)
    g_variant_builder_add (optbuilder, "{s@v}",
                           "xa.title",
                           g_variant_new_variant (g_variant_new_string (title)));

  if (!ostree_repo_remote_change (xdg_app_dir_get_repo (dir), NULL,
                                  opt_if_not_exists ? OSTREE_REPO_REMOTE_CHANGE_ADD_IF_NOT_EXISTS :
                                  OSTREE_REPO_REMOTE_CHANGE_ADD,
                                  remote_name, remote_url,
                                  g_variant_builder_end (optbuilder),
                                  cancellable, error))
    goto out;

  ret = TRUE;

 out:
  if (context)
    g_option_context_free (context);
  return ret;
}
