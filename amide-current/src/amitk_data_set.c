/* amitk_data_set.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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

#include "amide_config.h"

#include "amitk_data_set.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"

/* variable type function declarations */
#include "amitk_data_set_UBYTE_0D_SCALING.h"
#include "amitk_data_set_UBYTE_1D_SCALING.h"
#include "amitk_data_set_UBYTE_2D_SCALING.h"
#include "amitk_data_set_SBYTE_0D_SCALING.h"
#include "amitk_data_set_SBYTE_1D_SCALING.h"
#include "amitk_data_set_SBYTE_2D_SCALING.h"
#include "amitk_data_set_USHORT_0D_SCALING.h"
#include "amitk_data_set_USHORT_1D_SCALING.h"
#include "amitk_data_set_USHORT_2D_SCALING.h"
#include "amitk_data_set_SSHORT_0D_SCALING.h"
#include "amitk_data_set_SSHORT_1D_SCALING.h"
#include "amitk_data_set_SSHORT_2D_SCALING.h"
#include "amitk_data_set_UINT_0D_SCALING.h"
#include "amitk_data_set_UINT_1D_SCALING.h"
#include "amitk_data_set_UINT_2D_SCALING.h"
#include "amitk_data_set_SINT_0D_SCALING.h"
#include "amitk_data_set_SINT_1D_SCALING.h"
#include "amitk_data_set_SINT_2D_SCALING.h"
#include "amitk_data_set_FLOAT_0D_SCALING.h"
#include "amitk_data_set_FLOAT_1D_SCALING.h"
#include "amitk_data_set_FLOAT_2D_SCALING.h"
#include "amitk_data_set_DOUBLE_0D_SCALING.h"
#include "amitk_data_set_DOUBLE_1D_SCALING.h"
#include "amitk_data_set_DOUBLE_2D_SCALING.h"

#include <sys/time.h>
#include <time.h>
#ifdef AMIDE_DEBUG
#include <sys/timeb.h>
#endif
#include "raw_data_import.h"
#include "idl_data_import.h"
#include "cti_import.h"
#include "medcon_import.h"


#define EMPTY 0.0


/* external variables */
gchar * amitk_interpolation_explanations[] = {
  "interpolate using nearest neighbhor (fast)", 
  "interpolate using trilinear interpolation (slow)"
};


gchar * amitk_import_menu_names[] = {
  "",
  "_Raw Data", 
  "_IDL Output",
#ifdef AMIDE_LIBECAT_SUPPORT
  "_CTI 6.4/7.0 via libecat",
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  "(_X)medcon importing"
#endif
};
  

gchar * amitk_import_menu_explanations[] = {
  "",
  "Import file as raw data",
  "Import an IDL created file, generally from the microCT's volume rendering tool",
  "Import a CTI 6.4 or 7.0 file using the libecat library",
  "Import via the (X)medcon importing library (libmdc)",
};




enum {
  THRESHOLDING_CHANGED,
  COLOR_TABLE_CHANGED,
  SCALE_FACTOR_CHANGED,
  MODALITY_CHANGED,
  TIME_CHANGED,
  VOXEL_SIZE_CHANGED,
  DATA_SET_CHANGED,
  LAST_SIGNAL
};

static void          data_set_class_init          (AmitkDataSetClass *klass);
static void          data_set_init                (AmitkDataSet      *data_set);
static void          data_set_finalize            (GObject          *object);
static AmitkObject * data_set_copy                (const AmitkObject * object);
static void          data_set_copy_in_place       (AmitkObject * dest_object, const AmitkObject * src_object);
static void          data_set_write_xml           (const AmitkObject * object, xmlNodePtr nodes);
static void          data_set_read_xml            (AmitkObject * object, xmlNodePtr nodes);
static AmitkVolume * parent_class;
static guint         data_set_signals[LAST_SIGNAL];



GType amitk_data_set_get_type(void) {

  static GType data_set_type = 0;

  if (!data_set_type)
    {
      static const GTypeInfo data_set_info =
      {
	sizeof (AmitkDataSetClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) data_set_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkDataSet),
	0,			/* n_preallocs */
	(GInstanceInitFunc) data_set_init,
	NULL /* value table */
      };
      
      data_set_type = g_type_register_static (AMITK_TYPE_VOLUME, "AmitkDataSet", &data_set_info, 0);
    }

  return data_set_type;
}


static void data_set_class_init (AmitkDataSetClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  AmitkObjectClass * object_class = AMITK_OBJECT_CLASS(class);

  parent_class = g_type_class_peek_parent(class);

  object_class->object_copy = data_set_copy;
  object_class->object_copy_in_place = data_set_copy_in_place;
  object_class->object_write_xml = data_set_write_xml;
  object_class->object_read_xml = data_set_read_xml;

  gobject_class->finalize = data_set_finalize;

  data_set_signals[THRESHOLDING_CHANGED] =
    g_signal_new ("thresholding_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, thresholding_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[COLOR_TABLE_CHANGED] =
    g_signal_new ("color_table_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, color_table_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[SCALE_FACTOR_CHANGED] = 
    g_signal_new ("scale_factor_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, scale_factor_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[MODALITY_CHANGED] = 
    g_signal_new ("modality_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, modality_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[TIME_CHANGED] = 
    g_signal_new ("time_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, time_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[VOXEL_SIZE_CHANGED] = 
    g_signal_new ("voxel_size_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, voxel_size_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[DATA_SET_CHANGED] = 
    g_signal_new ("data_set_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, data_set_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
}


static void data_set_init (AmitkDataSet * data_set) {

  time_t current_time;
  gint i;

  /* put in some sensable values */
  data_set->raw_data = NULL;
  data_set->current_scaling = NULL;
  data_set->frame_duration = NULL;
  data_set->frame_max = NULL;
  data_set->frame_min = NULL;
  data_set->global_max = 0.0;
  data_set->global_min = 0.0;
  amitk_data_set_set_thresholding(data_set, AMITK_THRESHOLDING_GLOBAL);
  for (i=0; i<2; i++)
    data_set->threshold_ref_frame[i]=0;
  data_set->distribution = NULL;
  data_set->modality = AMITK_MODALITY_PET;
  data_set->voxel_size = zero_point;
  data_set->internal_scaling = amitk_raw_data_FLOAT_0D_SCALING_init(1.0);
  g_assert(data_set->internal_scaling!=NULL);
  amitk_data_set_set_scale_factor(data_set, 1.0);
  data_set->scan_start = 0.0;
  amitk_data_set_set_color_table(data_set, AMITK_COLOR_TABLE_BW_LINEAR);
  data_set->slice_parent = NULL;

  /* set the scan date to the current time, good for an initial guess */
  time(&current_time);
  data_set->scan_date = NULL;
  amitk_data_set_set_scan_date(data_set, ctime(&current_time));

}


static void data_set_finalize (GObject *object)
{
  AmitkDataSet * data_set = AMITK_DATA_SET(object);

  if (data_set->raw_data != NULL) {
#ifdef AMIDE_DEBUG
    if (data_set->raw_data->dim.z != 1) /* avoid slices */
      g_print("\tfreeing data set: %s\n",AMITK_OBJECT_NAME(data_set));
#endif
    g_object_unref(data_set->raw_data);
    data_set->raw_data = NULL;
  }

  if (data_set->internal_scaling != NULL) {
    g_object_unref(data_set->internal_scaling);
    data_set->internal_scaling = NULL;
  }

  if (data_set->current_scaling != NULL) {
    g_object_unref(data_set->current_scaling);
    data_set->current_scaling = NULL;
  }

  if (data_set->distribution != NULL) {
    g_object_unref(data_set->distribution);
    data_set->distribution = NULL;
  }

  if (data_set->frame_duration != NULL) {
    g_free(data_set->frame_duration);
    data_set->frame_duration = NULL;
  }

  if (data_set->frame_max != NULL) {
    g_free(data_set->frame_max);
    data_set->frame_max = NULL;
  }

  if (data_set->frame_min != NULL) {
    g_free(data_set->frame_min);
    data_set->frame_min = NULL;
  }

  if (data_set->scan_date != NULL) {
    g_free(data_set->scan_date);
    data_set->scan_date = NULL;
  }

  if (data_set->slice_parent != NULL) {
    g_object_unref(data_set->slice_parent);
    data_set->slice_parent = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static AmitkObject * data_set_copy (const AmitkObject * object) {

  AmitkDataSet * copy;

  g_return_val_if_fail(AMITK_IS_DATA_SET(object), NULL);
  
  copy = amitk_data_set_new();
  amitk_object_copy_in_place(AMITK_OBJECT(copy), object);

  return AMITK_OBJECT(copy);
}

/* Notes: 
   - does not make a copy of the source raw_data, just adds a reference 
   - does not make a copy of the distribution data, just adds a reference
   - does not make a copy of the internal scaling factor, just adds a reference */
static void data_set_copy_in_place (AmitkObject * dest_object, const AmitkObject * src_object) {

  AmitkDataSet * src_ds;
  AmitkDataSet * dest_ds;
  guint i;

  g_return_if_fail(AMITK_IS_DATA_SET(src_object));
  g_return_if_fail(AMITK_IS_DATA_SET(dest_object));

  src_ds = AMITK_DATA_SET(src_object);
  dest_ds = AMITK_DATA_SET(dest_object);


  /* copy the data elements */
  amitk_data_set_set_scan_date(dest_ds, AMITK_DATA_SET_SCAN_DATE(src_object));
  amitk_data_set_set_modality(dest_ds, AMITK_DATA_SET_MODALITY(src_object));
  dest_ds->voxel_size = AMITK_DATA_SET_VOXEL_SIZE(src_object);
  if (src_ds->raw_data != NULL) {
    if (dest_ds->raw_data != NULL)
      g_object_unref(dest_ds->raw_data);
    dest_ds->raw_data = g_object_ref(src_ds->raw_data);
  }

  /* just reference, as internal scaling is never suppose to change */
  if (src_ds->internal_scaling != NULL) {
    if (dest_ds->internal_scaling != NULL)
      g_object_unref(dest_ds->internal_scaling);
    dest_ds->internal_scaling = g_object_ref(src_ds->internal_scaling);
  }

  dest_ds->scan_start = AMITK_DATA_SET_SCAN_START(src_object);

  if (src_ds->distribution != NULL) {
    if (dest_ds->distribution != NULL)
      g_object_unref(dest_ds->distribution);
    dest_ds->distribution = g_object_ref(src_ds->distribution);
  }
  amitk_data_set_set_scale_factor(dest_ds, AMITK_DATA_SET_SCALE_FACTOR(src_object));

  amitk_data_set_set_color_table(dest_ds, AMITK_DATA_SET_COLOR_TABLE(src_object));
  amitk_data_set_set_thresholding(dest_ds,AMITK_DATA_SET_THRESHOLDING(src_object));
  for (i=0; i<2; i++) {
    amitk_data_set_set_threshold_min(dest_ds, i, AMITK_DATA_SET_THRESHOLD_MIN(src_object, i));
    amitk_data_set_set_threshold_max(dest_ds, i, AMITK_DATA_SET_THRESHOLD_MAX(src_object, i));
    amitk_data_set_set_threshold_ref_frame(dest_ds, i, AMITK_DATA_SET_THRESHOLD_REF_FRAME(src_object, i));
  }
  dest_ds->global_max = AMITK_DATA_SET_GLOBAL_MAX(src_object);
  dest_ds->global_min = AMITK_DATA_SET_GLOBAL_MIN(src_object);

  /* make a separate copy in memory of the volume's frame durations and max/min values */
  if (dest_ds->frame_duration != NULL) 
    g_free(dest_ds->frame_duration);
  dest_ds->frame_duration = amitk_data_set_get_frame_duration_mem(dest_ds);
  g_return_if_fail(dest_ds->frame_duration != NULL);
  for (i=0;i<AMITK_DATA_SET_NUM_FRAMES(dest_ds);i++)
    amitk_data_set_set_frame_duration(dest_ds, i,
				      src_ds->frame_duration[i]);

  if (dest_ds->frame_max != NULL)
    g_free(dest_ds->frame_max);
  dest_ds->frame_max = amitk_data_set_get_frame_max_min_mem(dest_ds);
  g_return_if_fail(dest_ds->frame_max != NULL);
  for (i=0;i<AMITK_DATA_SET_NUM_FRAMES(dest_ds);i++) 
    dest_ds->frame_max[i] = src_ds->frame_max[i];

  if (dest_ds->frame_min != NULL)
    g_free(dest_ds->frame_min);
  dest_ds->frame_min = amitk_data_set_get_frame_max_min_mem(dest_ds);
  g_return_if_fail(dest_ds->frame_min != NULL);
  for (i=0;i<AMITK_DATA_SET_NUM_FRAMES(dest_ds);i++)
    dest_ds->frame_min[i] = src_ds->frame_min[i];

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}







static void data_set_write_xml(const AmitkObject * object, xmlNodePtr nodes) {

  AmitkDataSet * ds;
  gchar * xml_filename;
  gchar * name;

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes);

  ds = AMITK_DATA_SET(object);

  xml_save_string(nodes, "scan_date", AMITK_DATA_SET_SCAN_DATE(ds));
  xml_save_string(nodes, "modality", amitk_modality_get_name(AMITK_DATA_SET_MODALITY(ds)));
  amitk_point_write_xml(nodes, "voxel_size", AMITK_DATA_SET_VOXEL_SIZE(ds));

  name = g_strdup_printf("data-set_%s_raw-data",AMITK_OBJECT_NAME(object));
  xml_filename = amitk_raw_data_write_xml(AMITK_DATA_SET_RAW_DATA(ds), name);
  g_free(name);
  xml_save_string(nodes, "raw_data_file", xml_filename);
  g_free(xml_filename);

   name = g_strdup_printf("data-set_%s_scaling-factors",AMITK_OBJECT_NAME(ds));
  xml_filename = amitk_raw_data_write_xml(ds->internal_scaling, name);
  g_free(name);
  xml_save_string(nodes, "internal_scaling_file", xml_filename);
  g_free(xml_filename);

  xml_save_data(nodes, "scale_factor", AMITK_DATA_SET_SCALE_FACTOR(ds));
  xml_save_time(nodes, "scan_start", AMITK_DATA_SET_SCAN_START(ds));
  xml_save_times(nodes, "frame_duration", ds->frame_duration, AMITK_DATA_SET_NUM_FRAMES(ds));
  xml_save_string(nodes, "color_table", amitk_color_table_get_name(AMITK_DATA_SET_COLOR_TABLE(ds)));
  xml_save_string(nodes, "thresholding", amitk_thresholding_get_name(AMITK_DATA_SET_THRESHOLDING(ds)));
  xml_save_data(nodes, "threshold_max_0", ds->threshold_max[0]);
  xml_save_data(nodes, "threshold_min_0", ds->threshold_min[0]);
  xml_save_int(nodes, "threshold_ref_frame_0", ds->threshold_ref_frame[0]);
  xml_save_data(nodes, "threshold_max_1", ds->threshold_max[1]);
  xml_save_data(nodes, "threshold_min_1", ds->threshold_min[1]);
  xml_save_int(nodes, "threshold_ref_frame_1", ds->threshold_ref_frame[1]);

  return;
}

static void data_set_read_xml(AmitkObject * object, xmlNodePtr nodes) {

  AmitkDataSet * ds;
  AmitkModality i_modality;
  AmitkColorTable i_color_table;
  AmitkThresholding i_thresholding;
  gchar * temp_string;
  gchar * scan_date;
  gchar * filename;

  AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes);

  ds = AMITK_DATA_SET(object);

  scan_date = xml_get_string(nodes, "scan_date");
  amitk_data_set_set_scan_date(ds, scan_date);
  g_free(scan_date);

  temp_string = xml_get_string(nodes, "modality");
  if (temp_string != NULL)
    for (i_modality=0; i_modality < AMITK_MODALITY_NUM; i_modality++) 
      if (g_strcasecmp(temp_string, amitk_modality_get_name(i_modality)) == 0)
	amitk_data_set_set_modality(ds, i_modality);
  g_free(temp_string);

  ds->voxel_size = amitk_point_read_xml(nodes, "voxel_size");

  filename = xml_get_string(nodes, "raw_data_file");
  ds->raw_data = amitk_raw_data_read_xml(filename);
  g_free(filename);

   filename = xml_get_string(nodes, "internal_scaling_file");
  ds->internal_scaling = amitk_raw_data_read_xml(filename);
  g_free(filename);

  amitk_data_set_set_scale_factor(ds, xml_get_data(nodes, "scale_factor"));
  ds->scan_start = xml_get_time(nodes, "scan_start");
  ds->frame_duration = xml_get_times(nodes, "frame_duration", AMITK_DATA_SET_NUM_FRAMES(ds));

  temp_string = xml_get_string(nodes, "color_table");
  if (temp_string != NULL)
    for (i_color_table=0; i_color_table < AMITK_COLOR_TABLE_NUM; i_color_table++) 
      if (g_strcasecmp(temp_string, amitk_color_table_get_name(i_color_table)) == 0)
	amitk_data_set_set_color_table(ds, i_color_table);
  g_free(temp_string);

  temp_string = xml_get_string(nodes, "thresholding");
  if (temp_string != NULL)
    for (i_thresholding=0; i_thresholding < AMITK_THRESHOLDING_NUM; i_thresholding++) 
      if (g_strcasecmp(temp_string, amitk_thresholding_get_name(i_thresholding)) == 0)
	amitk_data_set_set_thresholding(ds, i_thresholding);
  g_free(temp_string);

  ds->threshold_max[0] =  xml_get_data(nodes, "threshold_max_0");
  ds->threshold_max[1] =  xml_get_data(nodes, "threshold_max_1");
  ds->threshold_ref_frame[0] = xml_get_int(nodes,"threshold_ref_frame_0");
  ds->threshold_min[0] =  xml_get_data(nodes, "threshold_min_0");
  ds->threshold_min[1] =  xml_get_data(nodes, "threshold_min_1");
  ds->threshold_ref_frame[1] = xml_get_int(nodes,"threshold_ref_frame_1");

  /* recalc the temporary parameters */
  amitk_data_set_calc_far_corner(ds);
  amitk_data_set_calc_frame_max_min(ds);
  amitk_data_set_calc_global_max_min(ds);

  return;
}


AmitkDataSet * amitk_data_set_new (void) {

  AmitkDataSet * data_set;

  data_set = g_object_new(amitk_data_set_get_type(), NULL);

  return data_set;
}






/* reads the contents of a raw data file into an amide data set structure,
   note: returned structure will have 1 second frame durations entered
   note: returned structure will have a voxel size of 1x1x1 mm
   note: file_offset is bytes for a binary file, lines for an ascii file
*/
AmitkDataSet * amitk_data_set_import_raw_file(const gchar * file_name, 
					      AmitkRawFormat raw_format,
					      AmitkVoxel data_dim,
					      guint file_offset) {

  guint t;
  AmitkDataSet * ds;

  if ((ds = amitk_data_set_new()) == NULL) {
    g_warning("couldn't allocate space for the data set structure to hold data");
    return NULL;
  }

  /* read in the data set */
  ds->raw_data =  amitk_raw_data_import_raw_file(file_name, raw_format, data_dim, file_offset);
  if (ds->raw_data == NULL) {
    g_warning("raw_data_read_file failed returning NULL data set");
    g_object_unref(ds);
    return NULL;
  }

  /* allocate space for the array containing info on the duration of the frames */
  if ((ds->frame_duration = amitk_data_set_get_frame_duration_mem(ds)) == NULL) {
    g_warning("couldn't allocate space for the frame duration info");
    g_object_unref(ds);
    return NULL;
  }

  /* put in fake frame durations */
  for (t=0; t < AMITK_DATA_SET_NUM_FRAMES(ds); t++)
    amitk_data_set_set_frame_duration(ds, t, 1.0);

  /* calc max/min values */
  amitk_data_set_calc_frame_max_min(ds);
  amitk_data_set_calc_global_max_min(ds);

  /* put in fake voxel size values and set the far corner appropriately */
  amitk_data_set_set_modality(ds, AMITK_MODALITY_CT); /* guess CT... */
  ds->voxel_size = one_point;
  amitk_data_set_calc_far_corner(ds);

  /* set any remaining parameters */
  amitk_volume_set_center(AMITK_VOLUME(ds), zero_point);

  return ds;
}



/* function to import a file into a data set */
AmitkDataSet * amitk_data_set_import_file(AmitkImportMethod import_method, int submethod,
					  const gchar * import_filename) {
  AmitkDataSet * import_ds;
  gchar * import_filename_base;
  gchar * import_filename_extension;
  gchar ** frags=NULL;

  g_return_val_if_fail(import_filename != NULL, NULL);

  /* if we're guessing how to import.... */
  if (import_method == AMITK_IMPORT_METHOD_GUESS) {

    /* extract the extension of the file */
    import_filename_base = g_strdup(g_basename(import_filename));
    g_strreverse(import_filename_base);
    frags = g_strsplit(import_filename_base, ".", 2);
    g_free(import_filename_base);

    import_filename_extension = g_strdup(frags[0]);
    g_strreverse(import_filename_extension);
    g_strfreev(frags);
    
    if ((g_strcasecmp(import_filename_extension, "dat")==0) ||
	(g_strcasecmp(import_filename_extension, "raw")==0))  
      /* .dat and .raw are assumed to be raw data */
      import_method = AMITK_IMPORT_METHOD_RAW;
    else if (g_strcasecmp(import_filename_extension, "idl")==0)
      /* if it appears to be some sort of an idl file */
      import_method = AMITK_IMPORT_METHOD_IDL;
#ifdef AMIDE_LIBECAT_SUPPORT      
    else if ((g_strcasecmp(import_filename_extension, "img")==0) ||
	     (g_strcasecmp(import_filename_extension, "v")==0) ||
	     (g_strcasecmp(import_filename_extension, "atn")==0) ||
	     (g_strcasecmp(import_filename_extension, "scn")==0))
      /* if it appears to be a cti file */
      import_method = AMITK_IMPORT_METHOD_LIBECAT;
#endif
    else /* fallback methods */
#ifdef AMIDE_LIBMDC_SUPPORT
      /* try passing it to the libmdc library.... */
      import_method = AMITK_IMPORT_METHOD_LIBMDC;
#else
    { /* unrecognized file type */
      g_warning("Extension %s not recognized on file: %s\nGuessing File Type", 
		import_filename_extension, import_filename);
      import_method = AMITK_IMPORT_METHOD_RAW;
    }
#endif

    g_free(import_filename_extension);
  }    

  switch (import_method) {

  case AMITK_IMPORT_METHOD_IDL:
    if ((import_ds=idl_data_import(import_filename)) == NULL)
      g_warning("Could not interpret as an IDL file:\n\t%s",import_filename);
    break;
    
#ifdef AMIDE_LIBECAT_SUPPORT      
  case AMITK_IMPORT_METHOD_LIBECAT:
    if ((import_ds =cti_import(import_filename)) == NULL) 
      g_warning("Could not interpret as a CTI file using libecat:\n\t%s",import_filename);
    break;
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  case AMITK_IMPORT_METHOD_LIBMDC:
    if ((import_ds=medcon_import(import_filename, submethod)) == NULL) 
      g_warning("Could not interpret using (X)medcon/libmdc file:\n\t%s",import_filename);
    break;
#endif
  case AMITK_IMPORT_METHOD_RAW:
  default:
    if ((import_ds= raw_data_import(import_filename)) == NULL)
      g_warning("Could not interpret as a raw data file:\n\t%s", import_filename);
    break;
  }
  g_return_val_if_fail(import_ds != NULL, NULL);

  /* set the thresholds */
  import_ds->threshold_max[0] = import_ds->threshold_max[1] = import_ds->global_max;
  import_ds->threshold_min[0] = import_ds->threshold_min[1] =
    (import_ds->global_min > 0.0) ? import_ds->global_min : 0.0;
  import_ds->threshold_ref_frame[1] = AMITK_DATA_SET_NUM_FRAMES(import_ds)-1;

  return import_ds;
}

void amitk_data_set_set_modality(AmitkDataSet * ds, const AmitkModality modality) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if ( ds->modality != modality) {
    ds->modality = modality;
    g_signal_emit (G_OBJECT (ds), data_set_signals[MODALITY_CHANGED], 0);
    g_signal_emit (G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}


void amitk_data_set_set_scan_start (AmitkDataSet * ds, const amide_time_t start) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (ds->scan_start != start) {
    ds->scan_start = start;
    g_signal_emit (G_OBJECT (ds), data_set_signals[TIME_CHANGED], 0);
    g_signal_emit (G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}


void amitk_data_set_set_frame_duration(AmitkDataSet * ds, const guint frame,
				       const amide_time_t duration) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(frame < AMITK_DATA_SET_NUM_FRAMES(ds));

  if (ds->frame_duration[frame] != duration) {
    ds->frame_duration[frame] = duration;
    g_signal_emit (G_OBJECT (ds), data_set_signals[TIME_CHANGED], 0);
    g_signal_emit (G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}

void amitk_data_set_set_voxel_size(AmitkDataSet * ds, const AmitkPoint voxel_size) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (!POINT_EQUAL(AMITK_DATA_SET_VOXEL_SIZE(ds), voxel_size)) {
    ds->voxel_size = voxel_size;
    amitk_data_set_calc_far_corner(ds);
    g_signal_emit (G_OBJECT (ds), data_set_signals[VOXEL_SIZE_CHANGED], 0);
    g_signal_emit (G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}

void amitk_data_set_set_thresholding(AmitkDataSet * ds, const AmitkThresholding thresholding) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (ds->thresholding != thresholding) {
    ds->thresholding = thresholding;
    g_signal_emit (G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }
}

void amitk_data_set_set_threshold_max(AmitkDataSet * ds, guint which_reference, amide_data_t value) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (ds->threshold_max[which_reference] != value) {
    ds->threshold_max[which_reference] = value;
    g_signal_emit (G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }
}

void amitk_data_set_set_threshold_min(AmitkDataSet * ds, guint which_reference, amide_data_t value) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (ds->threshold_min[which_reference] != value ) {
    ds->threshold_min[which_reference] = value;
    g_signal_emit (G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }
}

void amitk_data_set_set_threshold_ref_frame(AmitkDataSet * ds, guint which_reference, guint frame) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (ds->threshold_ref_frame[which_reference] != frame) {
    ds->threshold_ref_frame[which_reference] = frame;
    g_signal_emit (G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }
}

void amitk_data_set_set_color_table(AmitkDataSet * ds, const AmitkColorTable new_color_table) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (new_color_table != ds->color_table) {
    ds->color_table = new_color_table;
    g_signal_emit (G_OBJECT (ds), data_set_signals[COLOR_TABLE_CHANGED], 0);
  }
}

/* sets the creation date of a data set
   note: no error checking is done on the date, as of yet */
void amitk_data_set_set_scan_date(AmitkDataSet * ds, const gchar * new_date) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (ds->scan_date != NULL) {
    g_free(ds->scan_date); 
    ds->scan_date = NULL;
  }

  if (new_date != NULL) {
    ds->scan_date = g_strdup(new_date); 
    g_strdelimit(ds->scan_date, "\n", ' '); /* turns newlines to white space */
    g_strstrip(ds->scan_date); /* removes trailing and leading white space */
  } else {
    ds->scan_date = g_strdup("unknown");
  }

  return;
}

/* used when changing the external scale factor, so that we can keep a pregenerated scaling factor */
void amitk_data_set_set_scale_factor(AmitkDataSet * ds, amide_data_t new_scale_factor) {

  AmitkVoxel i;
  gint j;
  amide_data_t scaling;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(ds->internal_scaling != NULL);

  if (new_scale_factor <= 0.0) return;

  if ((ds->scale_factor != new_scale_factor) || 
      (ds->current_scaling == NULL) ||
      (ds->current_scaling->dim.x != ds->internal_scaling->dim.x) ||
      (ds->current_scaling->dim.y != ds->internal_scaling->dim.y) ||
      (ds->current_scaling->dim.z != ds->internal_scaling->dim.z) ||
      (ds->current_scaling->dim.t != ds->internal_scaling->dim.t)) {

    if (ds->current_scaling == NULL) /* first time */
      scaling = 1.0;
    else
      scaling = new_scale_factor/ds->scale_factor;

    ds->scale_factor = new_scale_factor;
    if (ds->current_scaling != NULL)
      if ((ds->current_scaling->dim.x != ds->internal_scaling->dim.x) ||
	  (ds->current_scaling->dim.y != ds->internal_scaling->dim.y) ||
	  (ds->current_scaling->dim.z != ds->internal_scaling->dim.z) ||
	  (ds->current_scaling->dim.t != ds->internal_scaling->dim.t)) {
	g_object_unref(ds->current_scaling);
	ds->current_scaling = NULL;
      }
    
    if (ds->current_scaling == NULL) {
      ds->current_scaling = amitk_raw_data_new();
      g_return_if_fail(ds->current_scaling != NULL);
      
      ds->current_scaling->dim = ds->internal_scaling->dim;
      ds->current_scaling->format = ds->internal_scaling->format;
      ds->current_scaling->data = amitk_raw_data_get_data_mem(ds->current_scaling);
    }
    
    for(i.t = 0; i.t < ds->current_scaling->dim.t; i.t++) 
      for (i.z = 0; i.z < ds->current_scaling->dim.z; i.z++) 
	for (i.y = 0; i.y < ds->current_scaling->dim.y; i.y++) 
	  for (i.x = 0; i.x < ds->current_scaling->dim.x; i.x++) {
	    *AMITK_RAW_DATA_FLOAT_POINTER(ds->current_scaling, i) = 
	      ds->scale_factor *
	      *AMITK_RAW_DATA_FLOAT_POINTER(ds->internal_scaling, i);
	  }
    
    /* adjust all thresholds and other variables so they remain constant 
       relative the change in scale factors */
    for (j=0; j<2; j++) {
      amitk_data_set_set_threshold_min(ds, j, scaling*AMITK_DATA_SET_THRESHOLD_MIN(ds, j));
      amitk_data_set_set_threshold_max(ds, j, scaling*AMITK_DATA_SET_THRESHOLD_MAX(ds, j));
    }
    ds->global_max *= scaling;
    ds->global_min *= scaling;
    if ((AMITK_DATA_SET_RAW_DATA(ds) != NULL) && (ds->frame_max != NULL) && (ds->frame_min != NULL))
      for (j=0; j < AMITK_DATA_SET_NUM_FRAMES(ds); j++) {
	ds->frame_max[j] *= scaling;
	ds->frame_min[j] *= scaling;
      }

    /* and emit the signal */
    g_signal_emit (G_OBJECT (ds), data_set_signals[SCALE_FACTOR_CHANGED], 0);
    g_signal_emit (G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }

  return;
}


/* returns the start time of the given frame */
amide_time_t amitk_data_set_get_start_time(const AmitkDataSet * ds, const guint frame) {

  amide_time_t time;
  guint i_frame;
  guint check_frame;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 0.0);

  if (frame >= AMITK_DATA_SET_NUM_FRAMES(ds))
    check_frame = AMITK_DATA_SET_NUM_FRAMES(ds)-1;
  else
    check_frame = frame;

  time = ds->scan_start;

  for(i_frame=0;i_frame<check_frame;i_frame++)
    time += ds->frame_duration[i_frame];

  return time;
}

/* returns the end time of a given frame */
amide_time_t amitk_data_set_get_end_time(const AmitkDataSet * ds, const guint frame) {

  guint check_frame;
  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);


  if (frame >= AMITK_DATA_SET_NUM_FRAMES(ds))
    check_frame = AMITK_DATA_SET_NUM_FRAMES(ds)-1;
  else
    check_frame = frame;

  return 
    amitk_data_set_get_start_time(ds, check_frame) + 
    ds->frame_duration[check_frame];
}


/* returns the frame that corresponds to the time */
guint amitk_data_set_get_frame(const AmitkDataSet * ds, const amide_time_t time) {

  amide_time_t start, end;
  guint i_frame;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 0);

  start = amitk_data_set_get_start_time(ds, 0);
  if (time <= start)
    return 0;

  for(i_frame=0; i_frame < AMITK_DATA_SET_NUM_FRAMES(ds); i_frame++) {
    start = amitk_data_set_get_start_time(ds, i_frame);
    end = amitk_data_set_get_end_time(ds, i_frame);
    if ((start <= time) && (time <= end))
      return i_frame;
  }

  /* must be past the end */
  return AMITK_DATA_SET_NUM_FRAMES(ds)-1;
}

amide_time_t amitk_data_set_get_frame_duration (const AmitkDataSet * ds, guint frame) {

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);

  if (frame >= AMITK_DATA_SET_NUM_FRAMES(ds))
    frame = AMITK_DATA_SET_NUM_FRAMES(ds)-1;

  return ds->frame_duration[frame];
}

/* return the minimal frame duration in this data set */
amide_time_t amitk_data_set_get_min_frame_duration(const AmitkDataSet * ds) {

  amide_time_t min_frame_duration;
  guint i_frame;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);

  min_frame_duration = ds->frame_duration[0];
  
  for(i_frame=1;i_frame<AMITK_DATA_SET_NUM_FRAMES(ds);i_frame++)
    if (ds->frame_duration[i_frame] < min_frame_duration)
      min_frame_duration = ds->frame_duration[i_frame];

  return min_frame_duration;
}

/* function to recalculate the far corner of a data set */
void amitk_data_set_calc_far_corner(AmitkDataSet * ds) {

  AmitkPoint new_point;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  POINT_MULT(ds->raw_data->dim, ds->voxel_size, new_point);
  amitk_volume_set_corner(AMITK_VOLUME(ds), new_point);

  return;
}

/* function to calculate the global max and min, 
   the frame max and mins need to already be calculated */
void amitk_data_set_calc_global_max_min(AmitkDataSet * ds) {

  guint i;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(ds->frame_max != NULL);
  g_return_if_fail(ds->frame_min != NULL);

  ds->global_max = ds->frame_max[0];
  ds->global_min = ds->frame_min[0];
  for (i=1; i<AMITK_DATA_SET_NUM_FRAMES(ds); i++) {
    if (ds->global_max < ds->frame_max[i]) 
      ds->global_max = ds->frame_max[i];
    if (ds->global_min > ds->frame_min[i])
      ds->global_min = ds->frame_min[i];
  }

#ifdef AMIDE_DEBUG
  g_print("\tglobal max %5.3f global min %5.3f\n",ds->global_max,ds->global_min);
#endif
   
  return;
}


/* function to calculate the max and min over the data frames */
void amitk_data_set_calc_frame_max_min(AmitkDataSet * ds) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(ds->raw_data != NULL);

  /* allocate the arrays if we haven't already */
  if (ds->frame_max == NULL) {
    ds->frame_max = amitk_data_set_get_frame_max_min_mem(ds);
    ds->frame_min = amitk_data_set_get_frame_max_min_mem(ds);
  }
  g_return_if_fail(ds->frame_max != NULL);
  g_return_if_fail(ds->frame_min != NULL);

  /* hand everything off to the data type specific function */
  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_UBYTE_2D_SCALING_calc_frame_max_min(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_UBYTE_1D_SCALING_calc_frame_max_min(ds);
    else 
      amitk_data_set_UBYTE_0D_SCALING_calc_frame_max_min(ds);
    break;
  case AMITK_FORMAT_SBYTE:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_SBYTE_2D_SCALING_calc_frame_max_min(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_SBYTE_1D_SCALING_calc_frame_max_min(ds);
    else 
      amitk_data_set_SBYTE_0D_SCALING_calc_frame_max_min(ds);
    break;
  case AMITK_FORMAT_USHORT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_USHORT_2D_SCALING_calc_frame_max_min(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_USHORT_1D_SCALING_calc_frame_max_min(ds);
    else 
      amitk_data_set_USHORT_0D_SCALING_calc_frame_max_min(ds);
    break;
  case AMITK_FORMAT_SSHORT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_SSHORT_2D_SCALING_calc_frame_max_min(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_SSHORT_1D_SCALING_calc_frame_max_min(ds);
    else 
      amitk_data_set_SSHORT_0D_SCALING_calc_frame_max_min(ds);
    break;
  case AMITK_FORMAT_UINT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_UINT_2D_SCALING_calc_frame_max_min(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_UINT_1D_SCALING_calc_frame_max_min(ds);
    else 
      amitk_data_set_UINT_0D_SCALING_calc_frame_max_min(ds);
    break;
  case AMITK_FORMAT_SINT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_SINT_2D_SCALING_calc_frame_max_min(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_SINT_1D_SCALING_calc_frame_max_min(ds);
    else 
      amitk_data_set_SINT_0D_SCALING_calc_frame_max_min(ds);
    break;
  case AMITK_FORMAT_FLOAT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_FLOAT_2D_SCALING_calc_frame_max_min(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_FLOAT_1D_SCALING_calc_frame_max_min(ds);
    else 
      amitk_data_set_FLOAT_0D_SCALING_calc_frame_max_min(ds);
    break;
  case AMITK_FORMAT_DOUBLE:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_DOUBLE_2D_SCALING_calc_frame_max_min(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_DOUBLE_1D_SCALING_calc_frame_max_min(ds);
    else 
      amitk_data_set_DOUBLE_0D_SCALING_calc_frame_max_min(ds);
    break;
  default:
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
    break;
  }

  return;
}


amide_data_t amitk_data_set_get_max(const AmitkDataSet * ds, 
				    const amide_time_t start, 
				    const amide_time_t duration) {

  guint start_frame;
  guint end_frame;
  guint i_frame;
  amide_time_t used_start, used_end;
  amide_time_t ds_start, ds_end;
  amide_data_t max;
  amide_data_t time_weight;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);
  g_return_val_if_fail(duration >= 0.0, 1.0);

  /* figure out what frames of this data set to use */
  used_start = start;
  start_frame = amitk_data_set_get_frame(ds, used_start);
  ds_start = amitk_data_set_get_start_time(ds, start_frame);

  used_end = start+duration;
  end_frame = amitk_data_set_get_frame(ds, used_end);
  ds_end = amitk_data_set_get_end_time(ds, end_frame);

  if (start_frame == end_frame) {
    max = ds->frame_max[start_frame];
  } else {
    if (ds_end < used_end)
      used_end = ds_end;
    if (ds_start > used_start)
      used_start = ds_start;

    max = 0;
    for (i_frame=start_frame; i_frame <= end_frame; i_frame++) {

      if (i_frame == start_frame)
	time_weight = (amitk_data_set_get_end_time(ds, start_frame)-used_start)/(used_end-used_start);
      else if (i_frame == end_frame)
	time_weight = (used_end-amitk_data_set_get_start_time(ds, end_frame))/(used_end-used_start);
      else
	time_weight = ds->frame_duration[i_frame]/(used_end-used_start);

      max += time_weight*ds->frame_max[i_frame];
    }
  }

  return max;
}

amide_data_t amitk_data_set_get_min(const AmitkDataSet * ds, 
				    const amide_time_t start, 
				    const amide_time_t duration) {

  guint start_frame;
  guint end_frame;
  guint i_frame;
  amide_time_t used_start, used_end;
  amide_time_t ds_start, ds_end;
  amide_data_t min;
  amide_data_t time_weight;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 0.0);
  g_return_val_if_fail(duration>=0.0, 0.0);

  /* figure out what frames of this data set to use */
  used_start = start;
  start_frame = amitk_data_set_get_frame(ds, used_start);
  ds_start = amitk_data_set_get_start_time(ds, start_frame);

  used_end = start+duration;
  end_frame = amitk_data_set_get_frame(ds, used_end);
  ds_end = amitk_data_set_get_end_time(ds, end_frame);

  if (start_frame == end_frame) {
    min = ds->frame_min[start_frame];
  } else {
    if (ds_end < used_end)
      used_end = ds_end;
    if (ds_start > used_start)
      used_start = ds_start;

    min = 0;
    for (i_frame=start_frame; i_frame <= end_frame; i_frame++) {

      if (i_frame == start_frame)
	time_weight = (amitk_data_set_get_end_time(ds, start_frame)-used_start)/(used_end-used_start);
      else if (i_frame == end_frame)
	time_weight = (used_end-amitk_data_set_get_start_time(ds, end_frame))/(used_end-used_start);
      else
	time_weight = ds->frame_duration[i_frame]/(used_end-used_start);

      min += time_weight*ds->frame_min[i_frame];
    }
  }

  return min;
}

/* figure out the min and max threshold values given a threshold type */
/* slice is only needed for THRESHOLDING_PER_SLICE */
/* valid values for start and duration only needed for THRESHOLDING_PER_FRAME
   and THRESHOLDING_INTERPOLATE_FRAMES */
void amitk_data_set_get_thresholding_max_min(const AmitkDataSet * ds, 
					     const AmitkDataSet * slice,
					     const amide_time_t start,
					     const amide_time_t duration,
					     amide_data_t * max, amide_data_t * min) {


  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  /* get the max/min values for thresholding */
  switch(AMITK_DATA_SET_THRESHOLDING(ds)) {

  case AMITK_THRESHOLDING_PER_SLICE: 
    {
      g_return_if_fail(AMITK_IS_DATA_SET(slice));
      amide_data_t slice_max,slice_min;
      amide_data_t threshold_range;
      
      /* find the slice's max and min, and then adjust these values to
       * correspond to the current threshold values */
      slice_max = slice->global_max;
      slice_min = slice->global_min;
      threshold_range = ds->global_max-ds->global_min;
      *max = ds->threshold_max[0]*(slice_max-slice_min)/threshold_range;
      *min = ds->threshold_min[0]*(slice_max-slice_min)/threshold_range;
    }
    break;
  case AMITK_THRESHOLDING_PER_FRAME:
    {
      amide_data_t frame_max,frame_min;
      amide_data_t threshold_range;

      frame_max = amitk_data_set_get_max(ds, start,duration);
      frame_min = amitk_data_set_get_min(ds, start,duration);
      threshold_range = ds->global_max-ds->global_min;
      *max = ds->threshold_max[0]*(frame_max-frame_min)/threshold_range;
      *min = ds->threshold_min[0]*(frame_max-frame_min)/threshold_range;
    }
    break;
  case AMITK_THRESHOLDING_INTERPOLATE_FRAMES:
    {
      guint middle_frame;
    
    
      if (ds->threshold_ref_frame[1]==ds->threshold_ref_frame[0]) {
	*max = ds->threshold_max[0];
	*min = ds->threshold_min[0];
      } else {
	
	middle_frame = amitk_data_set_get_frame(ds, start+duration/2.0);
	if (middle_frame < ds->threshold_ref_frame[0])
	  middle_frame = ds->threshold_ref_frame[0];
	else if (middle_frame > ds->threshold_ref_frame[1])
	  middle_frame = ds->threshold_ref_frame[1];
	
	*max=
	  (((ds->threshold_ref_frame[1]-middle_frame)*ds->threshold_max[0]+
	    (middle_frame-ds->threshold_ref_frame[0])*ds->threshold_max[1])
	   /(ds->threshold_ref_frame[1]-ds->threshold_ref_frame[0]));
	*min=
	  (((ds->threshold_ref_frame[1]-middle_frame)*ds->threshold_min[0]+
	    (middle_frame-ds->threshold_ref_frame[0])*ds->threshold_min[1])
	   /(ds->threshold_ref_frame[1]-ds->threshold_ref_frame[0]));
      }
    }
    break;
  case AMITK_THRESHOLDING_GLOBAL:
    *max = ds->threshold_max[0];
    *min = ds->threshold_min[0];
    break;
  default:
    g_warning("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }
 
  return;
}

  




/* generate the distribution array for a data set */
void amitk_data_set_calc_distribution(AmitkDataSet * ds) {


  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(ds->raw_data != NULL);

  /* hand everything off to the data type specific function */
  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_UBYTE_2D_SCALING_calc_distribution(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_UBYTE_1D_SCALING_calc_distribution(ds);
    else 
      amitk_data_set_UBYTE_0D_SCALING_calc_distribution(ds);
    break;
  case AMITK_FORMAT_SBYTE:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_SBYTE_2D_SCALING_calc_distribution(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_SBYTE_1D_SCALING_calc_distribution(ds);
    else 
      amitk_data_set_SBYTE_0D_SCALING_calc_distribution(ds);
    break;
  case AMITK_FORMAT_USHORT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_USHORT_2D_SCALING_calc_distribution(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_USHORT_1D_SCALING_calc_distribution(ds);
    else 
      amitk_data_set_USHORT_0D_SCALING_calc_distribution(ds);
    break;
  case AMITK_FORMAT_SSHORT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_SSHORT_2D_SCALING_calc_distribution(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_SSHORT_1D_SCALING_calc_distribution(ds);
    else 
      amitk_data_set_SSHORT_0D_SCALING_calc_distribution(ds);
    break;
  case AMITK_FORMAT_UINT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_UINT_2D_SCALING_calc_distribution(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_UINT_1D_SCALING_calc_distribution(ds);
    else 
      amitk_data_set_UINT_0D_SCALING_calc_distribution(ds);
    break;
  case AMITK_FORMAT_SINT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_SINT_2D_SCALING_calc_distribution(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_SINT_1D_SCALING_calc_distribution(ds);
    else 
      amitk_data_set_SINT_0D_SCALING_calc_distribution(ds);
    break;
  case AMITK_FORMAT_FLOAT:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_FLOAT_2D_SCALING_calc_distribution(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_FLOAT_1D_SCALING_calc_distribution(ds);
    else 
      amitk_data_set_FLOAT_0D_SCALING_calc_distribution(ds);
    break;
  case AMITK_FORMAT_DOUBLE:
    if (ds->internal_scaling->dim.z > 1) 
      amitk_data_set_DOUBLE_2D_SCALING_calc_distribution(ds);
    else if (ds->internal_scaling->dim.t > 1)
      amitk_data_set_DOUBLE_1D_SCALING_calc_distribution(ds);
    else 
      amitk_data_set_DOUBLE_0D_SCALING_calc_distribution(ds);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return;
}

  
amide_data_t amitk_data_set_get_value(const AmitkDataSet * ds, const AmitkVoxel i) {

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), EMPTY);
  
  if (!amitk_raw_data_includes_voxel(ds->raw_data, i)) return EMPTY;

  /* hand everything off to the data type specific function */
  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    if (ds->internal_scaling->dim.z > 1) 
      return AMITK_DATA_SET_UBYTE_2D_SCALING_CONTENTS(ds,i);
    else if (ds->internal_scaling->dim.t > 1)
      return AMITK_DATA_SET_UBYTE_1D_SCALING_CONTENTS(ds,i);
    else 
      return AMITK_DATA_SET_UBYTE_0D_SCALING_CONTENTS(ds,i);
    break;
  case AMITK_FORMAT_SBYTE:
    if (ds->internal_scaling->dim.z > 1) 
      return AMITK_DATA_SET_SBYTE_2D_SCALING_CONTENTS(ds,i);
    else if (ds->internal_scaling->dim.t > 1)
      return AMITK_DATA_SET_SBYTE_1D_SCALING_CONTENTS(ds,i);
    else 
      return AMITK_DATA_SET_SBYTE_0D_SCALING_CONTENTS(ds,i);
    break;
  case AMITK_FORMAT_USHORT:
    if (ds->internal_scaling->dim.z > 1) 
      return AMITK_DATA_SET_USHORT_2D_SCALING_CONTENTS(ds,i);
    else if (ds->internal_scaling->dim.t > 1)
      return AMITK_DATA_SET_USHORT_1D_SCALING_CONTENTS(ds,i);
    else 
      return AMITK_DATA_SET_USHORT_0D_SCALING_CONTENTS(ds,i);
    break;
  case AMITK_FORMAT_SSHORT:
    if (ds->internal_scaling->dim.z > 1) 
      return AMITK_DATA_SET_SSHORT_2D_SCALING_CONTENTS(ds,i);
    else if (ds->internal_scaling->dim.t > 1)
      return AMITK_DATA_SET_SSHORT_1D_SCALING_CONTENTS(ds,i);
    else 
      return AMITK_DATA_SET_SSHORT_0D_SCALING_CONTENTS(ds,i);
    break;
  case AMITK_FORMAT_UINT:
    if (ds->internal_scaling->dim.z > 1) 
      return AMITK_DATA_SET_UINT_2D_SCALING_CONTENTS(ds,i);
    else if (ds->internal_scaling->dim.t > 1)
      return AMITK_DATA_SET_UINT_1D_SCALING_CONTENTS(ds,i);
    else 
      return AMITK_DATA_SET_UINT_0D_SCALING_CONTENTS(ds,i);
    break;
  case AMITK_FORMAT_SINT:
    if (ds->internal_scaling->dim.z > 1) 
      return AMITK_DATA_SET_SINT_2D_SCALING_CONTENTS(ds,i);
    else if (ds->internal_scaling->dim.t > 1)
      return AMITK_DATA_SET_SINT_1D_SCALING_CONTENTS(ds,i);
    else 
      return AMITK_DATA_SET_SINT_0D_SCALING_CONTENTS(ds,i);
    break;
  case AMITK_FORMAT_FLOAT:
    if (ds->internal_scaling->dim.z > 1) 
      return AMITK_DATA_SET_FLOAT_2D_SCALING_CONTENTS(ds,i);
    else if (ds->internal_scaling->dim.t > 1)
      return AMITK_DATA_SET_FLOAT_1D_SCALING_CONTENTS(ds,i);
    else 
      return AMITK_DATA_SET_FLOAT_0D_SCALING_CONTENTS(ds,i);
    break;
  case AMITK_FORMAT_DOUBLE:
    if (ds->internal_scaling->dim.z > 1) 
      return AMITK_DATA_SET_DOUBLE_2D_SCALING_CONTENTS(ds,i);
    else if (ds->internal_scaling->dim.t > 1)
      return AMITK_DATA_SET_DOUBLE_1D_SCALING_CONTENTS(ds,i);
    else 
      return AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENTS(ds,i);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    return 0.0;
    break;
  }
}



/* returns a "2D" slice from a data set */
AmitkDataSet *amitk_data_set_get_slice(AmitkDataSet * ds,
				       const amide_time_t start,
				       const amide_time_t duration,
				       const AmitkPoint  requested_voxel_size,
				       const AmitkVolume * slice_volume,
				       const AmitkInterpolation interpolation,
				       const gboolean need_calc_max_min) {

  AmitkDataSet * slice;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), NULL);
  g_return_val_if_fail(ds->raw_data != NULL, NULL);

  /* hand everything off to the data type specific slicer */
  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    if (ds->internal_scaling->dim.z > 1) 
      slice = amitk_data_set_UBYTE_2D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    else if (ds->internal_scaling->dim.t > 1)
      slice = amitk_data_set_UBYTE_1D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    else 
      slice = amitk_data_set_UBYTE_0D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    break;
  case AMITK_FORMAT_SBYTE:
    if (ds->internal_scaling->dim.z > 1) 
      slice = amitk_data_set_SBYTE_2D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    else if (ds->internal_scaling->dim.t > 1)
      slice = amitk_data_set_SBYTE_1D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    else 
      slice = amitk_data_set_SBYTE_0D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    break;
  case AMITK_FORMAT_USHORT:
    if (ds->internal_scaling->dim.z > 1) 
      slice = amitk_data_set_USHORT_2D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    else if (ds->internal_scaling->dim.t > 1)
      slice = amitk_data_set_USHORT_1D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    else 
      slice = amitk_data_set_USHORT_0D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    break;
  case AMITK_FORMAT_SSHORT:
    if (ds->internal_scaling->dim.z > 1) 
      slice = amitk_data_set_SSHORT_2D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    else if (ds->internal_scaling->dim.t > 1)
      slice = amitk_data_set_SSHORT_1D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    else 
      slice = amitk_data_set_SSHORT_0D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    break;
  case AMITK_FORMAT_UINT:
    if (ds->internal_scaling->dim.z > 1) 
      slice = amitk_data_set_UINT_2D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
					       slice_volume, interpolation, need_calc_max_min);
    else if (ds->internal_scaling->dim.t > 1)
      slice = amitk_data_set_UINT_1D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
					       slice_volume, interpolation, need_calc_max_min);
    else 
      slice = amitk_data_set_UINT_0D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
					       slice_volume, interpolation, need_calc_max_min);
    break;
  case AMITK_FORMAT_SINT:
    if (ds->internal_scaling->dim.z > 1) 
      slice = amitk_data_set_SINT_2D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
					       slice_volume, interpolation, need_calc_max_min);
    else if (ds->internal_scaling->dim.t > 1)
      slice = amitk_data_set_SINT_1D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
					       slice_volume, interpolation, need_calc_max_min);
    else 
      slice = amitk_data_set_SINT_0D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
					       slice_volume, interpolation, need_calc_max_min);
    break;
  case AMITK_FORMAT_FLOAT:
    if (ds->internal_scaling->dim.z > 1) 
      slice = amitk_data_set_FLOAT_2D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    else if (ds->internal_scaling->dim.t > 1)
      slice = amitk_data_set_FLOAT_1D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    else 
      slice = amitk_data_set_FLOAT_0D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						slice_volume, interpolation, need_calc_max_min);
    break;
  case AMITK_FORMAT_DOUBLE:
    if (ds->internal_scaling->dim.z > 1) 
      slice = amitk_data_set_DOUBLE_2D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    else if (ds->internal_scaling->dim.t > 1)
      slice = amitk_data_set_DOUBLE_1D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    else 
      slice = amitk_data_set_DOUBLE_0D_SCALING_get_slice(ds, start, duration, requested_voxel_size,
						 slice_volume, interpolation, need_calc_max_min);
    break;
  default:
    slice = NULL;
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return slice;
}


gint amitk_data_sets_count(GList * objects, gboolean recurse) {

  gint count;

  if (objects == NULL) 
    return 0;

  if (AMITK_IS_DATA_SET(objects->data))
    count = 1;
  else
    count = 0;

  if (recurse) /* count data sets that are children */
    count += amitk_data_sets_count(AMITK_OBJECT_CHILDREN(objects->data), recurse);

  /* add this count too the counts from the rest of the objects */
  return count+amitk_data_sets_count(objects->next, recurse);
}



/* function to return the minimum frame duration from a list of objects */
amide_time_t amitk_data_sets_get_min_frame_duration(GList * objects) {

  amide_time_t min_duration, temp;

  if (objects == NULL) return -1.0; /* invalid */

  /* first process the rest of the list */
  min_duration = amitk_data_sets_get_min_frame_duration(objects->next);

  /* now process and compare to the children */
  temp = amitk_data_sets_get_min_frame_duration(AMITK_OBJECT_CHILDREN(objects->data));
  if (min_duration < 0.0) min_duration = temp;
  else if (temp < min_duration) min_duration = temp;
  
  /* and process this guy */
  if (AMITK_IS_DATA_SET(objects->data)) {
    temp = amitk_data_set_get_min_frame_duration(objects->data);
    if (min_duration < 0.0) min_duration = temp;
    else if (temp < min_duration) min_duration = temp;
  }

  return min_duration;
}



/* returns the minimum voxel dimensions of the list of data sets */
amide_real_t amitk_data_sets_get_min_voxel_size(GList * objects) {

  amide_real_t min_voxel_size, temp;

  if (objects == NULL) return -1.0; /* invalid */

  /* first process the rest of the list */
  min_voxel_size = amitk_data_sets_get_min_voxel_size(objects->next);

  /* now process and compare to the children */
  temp = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(objects->data));
  if (min_voxel_size < 0.0) min_voxel_size = temp;
  else if (temp < min_voxel_size) min_voxel_size = temp;

  /* and process this guy */
  if (AMITK_IS_DATA_SET(objects->data)) {
    temp = point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(objects->data));
    if (min_voxel_size < 0.0) min_voxel_size = temp;
    else if (temp < min_voxel_size) min_voxel_size = temp;
  }

  return min_voxel_size;
}




/* returns the minimum dimensional width of the data set with the largest voxel size */
/* figure out what our voxel size is going to be for our returned
   slices.  I'm going to base this on the volume with the largest
   minimum voxel dimension, this way the user doesn't suddenly get a
   huge image when adding in a study with small voxels (i.e. a CT
   scan).  The user can always increase the zoom later*/
amide_real_t amitk_data_sets_get_max_min_voxel_size(GList * objects) {

  amide_real_t min_voxel_size, temp;

  if (objects == NULL) return -1.0; /* invalid */

  /* first process the rest of the list */
  min_voxel_size = amitk_data_sets_get_max_min_voxel_size(objects->next);

  /* now process and compare to the children */
  temp = amitk_data_sets_get_max_min_voxel_size(AMITK_OBJECT_CHILDREN(objects->data));
  if (min_voxel_size < 0.0) min_voxel_size = temp;
  else if (temp > min_voxel_size) min_voxel_size = temp;

  /* and process this guy */
  if (AMITK_IS_DATA_SET(objects->data)) {
    temp = point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(objects->data));
    if (min_voxel_size < 0.0) min_voxel_size = temp;
    else if (temp > min_voxel_size) min_voxel_size = temp;
  }
  return min_voxel_size;
}

/* returns an unreferenced pointer to the slice in the list with the given parent */
AmitkDataSet * amitk_data_sets_find_with_slice_parent(GList * slices, const AmitkDataSet * slice_parent) {

  AmitkDataSet * slice;

  if (slices == NULL) return NULL;

  /* first process the rest of the list */
  slice = amitk_data_sets_find_with_slice_parent(slices->next, slice_parent);

  /* check children */
  if (slice == NULL)
    slice = amitk_data_sets_find_with_slice_parent(AMITK_OBJECT_CHILDREN(slices->data), slice_parent);

  /* check this guy */
  if (slice == NULL) 
    if (AMITK_IS_DATA_SET(slices->data))
      if (AMITK_DATA_SET(slices->data)->slice_parent == slice_parent)
	slice = AMITK_DATA_SET(slices->data);

  return slice;
}



/* give a list ov data_sets, returns a list of slices of equal size and orientation
   intersecting these data_sets */
/* thickness (of slice) in mm, negative indicates set this value according to voxel size */
GList * amitk_data_sets_get_slices(GList * objects,
				   const amide_time_t start,
				   const amide_time_t duration,
				   const amide_real_t pixel_dim,
				   const AmitkVolume * view_volume,
				   const AmitkInterpolation interpolation,
				   const gboolean need_calc_max_min) {


  AmitkPoint voxel_size;
  AmitkVolume * slice_volume;
  AmitkCorners view_corners;
  GList * slices=NULL;
  AmitkDataSet * slice;
#ifdef AMIDE_DEBUG
  struct timeval tv1;
  struct timeval tv2;
  gdouble time1;
  gdouble time2;
  gdouble total_time;
#endif

#ifdef AMIDE_DEBUG
  /* let's do some timing */
  gettimeofday(&tv1, NULL);
#endif

  g_return_val_if_fail(objects != NULL, NULL);

  voxel_size.x = voxel_size.y = pixel_dim;
  voxel_size.z = AMITK_VOLUME_Z_CORNER(view_volume);

  /* make valgrind happy */
  view_corners[0] = zero_point;
  view_corners[1] = zero_point;

  /* figure out all encompasing corners for the slices based on our viewing axis */
  amitk_volumes_get_enclosing_corners(objects, AMITK_SPACE(view_volume), view_corners);

  /* adjust the z' dimension of the view's corners */
  view_corners[0].z=0;
  view_corners[1].z=voxel_size.z;

  view_corners[0] = amitk_space_s2b(AMITK_SPACE(view_volume),view_corners[0]);
  view_corners[1] = amitk_space_s2b(AMITK_SPACE(view_volume),view_corners[1]);

  /* figure out the coordinate frame for the slices based on the corners returned */
  slice_volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(view_volume)));
  amitk_space_set_offset(AMITK_SPACE(slice_volume), view_corners[0]);
  amitk_volume_set_corner(slice_volume, amitk_space_b2s(AMITK_SPACE(slice_volume), 
							view_corners[1]));
  
  /* and get the slices */
  while (objects != NULL) {
    if (AMITK_IS_DATA_SET(objects->data)) {
      slice = 
	amitk_data_set_get_slice(AMITK_DATA_SET(objects->data), start, duration, voxel_size, 
				 slice_volume, interpolation, need_calc_max_min);
      if (slice != NULL)
	slices = g_list_append(slices, slice);
      else
	g_warning("slice inappropriately null in %s at %d", __FILE__, __LINE__);
    }
    objects = objects->next;
  }
  g_object_unref(slice_volume);
  g_return_val_if_fail(slices != NULL, NULL);

#ifdef AMIDE_DEBUG
  /* and wrapup our timing */
  gettimeofday(&tv2, NULL);
  time1 = ((double) tv1.tv_sec) + ((double) tv1.tv_usec)/1000000.0;
  time2 = ((double) tv2.tv_sec) + ((double) tv2.tv_usec)/1000000.0;
  total_time = time2-time1;
  g_print("######## Slice Generating Took %5.3f (s) #########\n",total_time);
#endif

  return slices;
}












const gchar * amitk_modality_get_name(const AmitkModality modality) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_MODALITY);
  enum_value = g_enum_get_value(enum_class, modality);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_interpolation_get_name(const AmitkModality modality) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_MODALITY);
  enum_value = g_enum_get_value(enum_class, modality);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_thresholding_get_name(const AmitkThresholding thresholding) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_THRESHOLDING);
  enum_value = g_enum_get_value(enum_class, thresholding);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}



