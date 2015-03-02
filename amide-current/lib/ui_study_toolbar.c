/* ui_study_toolbar.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
 *
 * Author: Andy Loening <loening@ucla.edu>
 */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.
*/




#include "config.h"
#include <gnome.h>
#include "study.h"
#include "rendering.h"
#include "amitk_spin_button.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_study_menus.h"
#include "ui_study_toolbar.h"
#include "../pixmaps/icon_threshold.xpm"
#include "../pixmaps/icon_interpolation_nearest_neighbor.xpm"
#include "../pixmaps/icon_interpolation_trilinear.xpm"
#include "../pixmaps/icon_view_single.xpm"
#include "../pixmaps/icon_view_linked.xpm"


      

/* function to setup the toolbar for the study ui */
void ui_study_toolbar_create(ui_study_t * ui_study) {

  interpolation_t i_interpolation;
  view_mode_t i_view_mode;
  GtkWidget * label;
  GtkWidget * toolbar;
  GtkObject * adjustment;
  gchar ** icon_interpolation[NUM_INTERPOLATIONS] = {icon_interpolation_nearest_neighbor_xpm,
						    icon_interpolation_trilinear_xpm};
  gchar ** icon_view_mode[NUM_VIEW_MODES] = {icon_view_single_xpm,
					     icon_view_linked_xpm};

  /* the toolbar definitions */
  GnomeUIInfo interpolation_list[NUM_INTERPOLATIONS+1];
  GnomeUIInfo view_mode_list[NUM_VIEW_MODES+1];

  GnomeUIInfo study_main_toolbar[] = {
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(interpolation_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(view_mode_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_DATA(NULL,
			  N_("Set the thresholds and colormaps for the active data set"),
			  ui_study_cb_threshold_pressed,
			  ui_study, icon_threshold_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  /* sanity check */
  g_assert(ui_study!=NULL);


  
  /* start make the interpolation toolbar items*/
  for (i_interpolation = 0; i_interpolation < NUM_INTERPOLATIONS; i_interpolation++)
    ui_study_menus_fill_in_radioitem(&(interpolation_list[i_interpolation]),
				     (icon_interpolation[i_interpolation] == NULL) ? 
				     interpolation_names[i_interpolation] : NULL,
				     interpolation_explanations[i_interpolation],
				     ui_study_cb_interpolation,
				     ui_study, 
				     icon_interpolation[i_interpolation]);
  ui_study_menus_fill_in_end(&(interpolation_list[NUM_INTERPOLATIONS]));

  /* and the view modes */
  for (i_view_mode = 0; i_view_mode < NUM_VIEW_MODES; i_view_mode++)
    ui_study_menus_fill_in_radioitem(&(view_mode_list[i_view_mode]),
				     (icon_view_mode[i_view_mode] == NULL) ? 
				     view_mode_names[i_view_mode] : NULL,
				     view_mode_explanations[i_view_mode],
				     ui_study_cb_view_mode,
				     ui_study, 
				     icon_view_mode[i_view_mode]);
  ui_study_menus_fill_in_end(&(view_mode_list[NUM_VIEW_MODES]));


  /* make the toolbar */
  toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,GTK_TOOLBAR_BOTH);
  gnome_app_fill_toolbar(GTK_TOOLBAR(toolbar), study_main_toolbar, NULL);




  /* finish setting up the interpolation items */
  for (i_interpolation = 0; i_interpolation < NUM_INTERPOLATIONS; i_interpolation++) {
    gtk_object_set_data(GTK_OBJECT(interpolation_list[i_interpolation].widget), 
			"interpolation", GINT_TO_POINTER(i_interpolation));
    gtk_signal_handler_block_by_func(GTK_OBJECT(interpolation_list[i_interpolation].widget),
				     GTK_SIGNAL_FUNC(ui_study_cb_interpolation), ui_study);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(interpolation_list[study_interpolation(ui_study->study)].widget),
			       TRUE);
  for (i_interpolation = 0; i_interpolation < NUM_INTERPOLATIONS; i_interpolation++)
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(interpolation_list[i_interpolation].widget),
				       GTK_SIGNAL_FUNC(ui_study_cb_interpolation),  ui_study);

  /* and the view modes */
  for (i_view_mode = 0; i_view_mode < NUM_VIEW_MODES; i_view_mode++) {
    gtk_object_set_data(GTK_OBJECT(view_mode_list[i_view_mode].widget), 
			"view_mode", GINT_TO_POINTER(i_view_mode));
    gtk_signal_handler_block_by_func(GTK_OBJECT(view_mode_list[i_view_mode].widget),
				     GTK_SIGNAL_FUNC(ui_study_cb_view_mode), ui_study);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view_mode_list[ui_study->view_mode].widget),
			       TRUE);
  for (i_view_mode = 0; i_view_mode < NUM_VIEW_MODES; i_view_mode++)
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(view_mode_list[i_view_mode].widget),
				       GTK_SIGNAL_FUNC(ui_study_cb_view_mode),ui_study);


  /* add the zoom widget to our toolbar */
  label = gtk_label_new("zoom:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(study_zoom(ui_study->study), 0.2,5,0.2, 0.25, 0.25);
  ui_study->zoom_spin = amitk_spin_button_new(GTK_ADJUSTMENT(adjustment), 0.25, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->zoom_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->zoom_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->zoom_spin), TRUE);
  gtk_widget_set_usize (ui_study->zoom_spin, 50, 0);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ui_study->zoom_spin), GTK_UPDATE_ALWAYS);

  gtk_signal_connect(adjustment, "value_changed",  GTK_SIGNAL_FUNC(ui_study_cb_zoom), ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), ui_study->zoom_spin, 
  			    "specify how much to magnify the images", NULL);
  gtk_widget_show(ui_study->zoom_spin);
			      
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));


  /* add the slice thickness selector */
  label = gtk_label_new("thickness:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(1.0,1.0,1.0,1.0,1.0,1.0);
  ui_study->thickness_spin = amitk_spin_button_new(GTK_ADJUSTMENT(adjustment),1.0, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->thickness_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->thickness_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->thickness_spin), TRUE);
  gtk_widget_set_usize (ui_study->thickness_spin, 50, 0);

  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ui_study->thickness_spin), 
				    GTK_UPDATE_ALWAYS);

  gtk_signal_connect(adjustment, "value_changed", 
		     GTK_SIGNAL_FUNC(ui_study_cb_thickness), ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), ui_study->thickness_spin, 
			    "specify how thick to make the slices (mm)", NULL);
  gtk_widget_show(ui_study->thickness_spin);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  /* frame selector */
  label = gtk_label_new("time:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  ui_study->time_button = gtk_button_new_with_label(""); 
  ui_study_update_time_button(ui_study); /* put in some meaningful text */

  gtk_signal_connect(GTK_OBJECT(ui_study->time_button), "pressed",
		     GTK_SIGNAL_FUNC(ui_study_cb_time_pressed), 
		     ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), 
  			    ui_study->time_button, "the time range over which to view the data (s)", NULL);
  gtk_widget_show(ui_study->time_button);






  /* add our toolbar to our app */
  gnome_app_set_toolbar(GNOME_APP(ui_study->app), GTK_TOOLBAR(toolbar));



  return;

}








