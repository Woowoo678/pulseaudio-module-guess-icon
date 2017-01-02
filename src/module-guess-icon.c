/***
    This file is part of pulseaudio-module-guess-icon.

    Copyright (C) 2017 Austin Steele

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
    USA
***/

/* This refers to the config.h file generated in the pulseaudio sources */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <gtk/gtk.h>
#include <pulsecore/core.h>

PA_MODULE_AUTHOR("Austin Steele");
PA_MODULE_DESCRIPTION("Guess application icon when one is not provided");
PA_MODULE_LOAD_ONCE(true);

struct userdata {
    pa_hook_slot *sink_input_put_slot;
    pa_hook_slot *source_output_put_slot;
};

/**** Check whether an icon name is valid */
static bool validate_icon(char *icon_name) {
	/* TODO: Figure out something for non-GTK environments */

    pa_assert(icon_name);

    gtk_init_check(0, NULL);
    bool valid = (gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(), icon_name, GTK_ICON_SIZE_MENU, 0)) ? (true) : (false);
    gtk_main_iteration_do(false);

    return valid;
}

/**** Try to find an icon name for a given proplist and return it, NULL if none found */
static char *search_icon(pa_proplist *props) {
    /* TODO: Smarter ways to search icons than just searching the application name
    	Check window manager properties?
    */
    pa_assert(props);

	char *icon_name = pa_xstrdup(pa_proplist_gets(pa_proplist_copy(props), PA_PROP_APPLICATION_NAME));

    if (icon_name) {
        for (int i = 0; icon_name[i]; i++) {
            icon_name[i] = tolower(icon_name[i]);
        }
    }

    if (icon_name && !validate_icon(icon_name))
        icon_name = NULL;

    return icon_name;
}

/**** Check whether a given proplist has any icon data */
static bool check_icon(pa_proplist *props) {
    pa_assert(props);

    if(pa_proplist_gets(pa_proplist_copy(props), PA_PROP_APPLICATION_ICON_NAME))
        return true;
    else if(pa_proplist_gets(pa_proplist_copy(props), PA_PROP_APPLICATION_ICON))
        return true;
    else
        return false;
}

/**** If proplist does not have an icon, try to find and set one */
static void process_props(pa_proplist *props) {
    pa_assert(props);

    if(!check_icon(props)) {
        char *icon_name = search_icon(props);

        if(icon_name)
            pa_proplist_sets(props, PA_PROP_APPLICATION_ICON_NAME, icon_name);

        pa_xfree(icon_name);
    }
}

static pa_hook_result_t sink_input_put_cb(pa_core *core, pa_sink_input *input, struct userdata *u) {
    pa_assert(core);
    pa_assert(input);
    pa_assert(u);

    process_props(input->proplist);
    return PA_HOOK_OK;
}

static pa_hook_result_t source_output_put_cb(pa_core *core, pa_source_output *output, struct userdata *u) {
    pa_assert(core);
    pa_assert(output);
    pa_assert(u);

    process_props(output->proplist);
    return PA_HOOK_OK;
}


int pa__init(pa_module* m) {
    struct userdata *u;
    pa_assert(m);

    m->userdata = u = pa_xnew(struct userdata, 1);

    u->sink_input_put_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_INPUT_PUT], PA_HOOK_LATE, (pa_hook_cb_t) sink_input_put_cb, u);
    u->source_output_put_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_PUT], PA_HOOK_LATE, (pa_hook_cb_t) source_output_put_cb, u);

    return 0;
}

void pa__done(pa_module* m) {
    struct userdata *u;
    pa_assert(m);

    if (!(u = m->userdata))
        return;
    
    if (u->sink_input_put_slot)
        pa_hook_slot_free(u->sink_input_put_slot);

    if (u->source_output_put_slot)
        pa_hook_slot_free(u->source_output_put_slot);

    pa_xfree(u);
}