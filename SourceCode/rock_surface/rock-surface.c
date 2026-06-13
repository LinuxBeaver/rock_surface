/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Credit to Øyvind Kolås (pippin) for major GEGL contributions
 * GEGL Rock Surface Texture, 2024 Beaver 
 */

/* 

Fun fact, this plugin is a fork of my other plugin Polygons; which has a very similar source code. 

GEGL Graph of Rock Surface to test without installing.

id=0
src aux=[ ref=0
cell-noise scale=0.122 shape=2 rank=1 iterations=11 palettize=no seed=211950323
emboss azimuth=30 elevation=30 depth=5
rgb-clip
id=1
multiply aux=[ color value=#9d6b14    ]   ]
crop
id=2
src-atop aux=[ ref=2 mantiuk06 opacity value=0.2 ]
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double  (scale, _("Scale of rock texture (in reverse)"), 0.122)
    description  (_("The scale of the internal cell noise function making a rock texture. Smaller scale makes the rock texture larger"))
    value_range  (0.05, 0.65)

property_seed    (seed, _("Random seed"), rand)
    description  (_("The random seed for the noise function"))

property_double (azimuth, _("Light rotation of rock texture"), 35.0)
    description (_("Light angle (degrees) of the internal emboss making a rock texture"))
    value_range (0, 360)
    ui_meta ("unit", "degree")
    ui_meta ("direction", "ccw")

property_double (elevation, _("Elevation of rock texture"), 30)
    description (_("Elevation of the internal emboss making a rock texture"))
    value_range (10, 50)

property_int (depth, _("Depth of rock texture"), 6)
    description (_("Depth of the internal emboss making a rock texture"))
    value_range (1, 25)

property_color (value, _("Color"), "#9d6b14")
    description (_("The color to paint over the input"))

property_boolean (switchm6, _("Enable Mantiuk06 tonemap"), TRUE)
    description (_("Rather the tone map Mantiuk06 should be on or off. It makes the filter slower but higher quality when enabled"))


property_double (tonemap, _("Tone map visibility"), 0.3)
    description (_("Presence of the mantiuk06 tone map"))
    value_range (0.0, 0.7)
  ui_meta     ("sensitive", "switchm6")

property_double (sldog, _("Soft light difference of gaussian visibility"), 0.5)
    description (_("Presence of soft light difference of gaussian blend"))
    value_range (0.0, 1.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     rock_surface
#define GEGL_OP_C_SOURCE rock-surface.c

#include "gegl-op.h"


typedef struct
{
  GeglNode *input;
  GeglNode *output;
  GeglNode *cellnoise;
  GeglNode *emboss;
  GeglNode *multiply;
  GeglNode *color;
  GeglNode *src;
  GeglNode *idrefx;
  GeglNode *rgbclip;
  GeglNode *crop;
  GeglNode *over;
  GeglNode *mantiuk06;
  GeglNode *nothing06;
  GeglNode *softlight;
  GeglNode *opacity;
  GeglNode *dog;

} State;


static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = g_malloc0 (sizeof (State));
  GeglColor *defaultcolor = gegl_color_new ("#9d6b14");
  o->user_data = state;

state->input    = gegl_node_get_input_proxy (gegl, "input");
state->output   = gegl_node_get_output_proxy (gegl, "output");

    state->cellnoise      = gegl_node_new_child (gegl, "operation", "gegl:cell-noise",
                                         "rank", 1, "shape", 2.0, "iterations", 11,  "palettize", FALSE,
                                         NULL);

    state->emboss = gegl_node_new_child (gegl,
                                  "operation", "gegl:emboss",
                                  NULL);

    state->multiply = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);

   state->color = gegl_node_new_child (gegl,
                                  "operation", "gegl:color", "value", defaultcolor,
                                  NULL);

    state->src = gegl_node_new_child (gegl,
                                  "operation", "gegl:src",
                                  NULL);

     
    state->rgbclip = gegl_node_new_child (gegl,
                                  "operation", "gegl:rgb-clip",
                                  NULL);

    state->crop = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);


    state->over = gegl_node_new_child (gegl,
                                  "operation", "gegl:over",
                                  NULL);

    state->opacity = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", 
                                  NULL);

   state->mantiuk06 = gegl_node_new_child (gegl,
                                  "operation", "gegl:mantiuk06", 
                                  NULL);

   state->nothing06 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop", 
                                  NULL);

    state->softlight = gegl_node_new_child (gegl,
                                  "operation", "gimp:layer-mode", "layer-mode", 45, 
                                  NULL);

    state->dog = gegl_node_new_child (gegl,
                                  "operation", "gegl:difference-of-gaussians", "radius1", 5.0, "radius2", 5.0,
                                  NULL);
}

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  GeglNode*mantiuk06 = state->nothing06;

{
if (!state) return;


if (o->switchm6)
mantiuk06 = state->mantiuk06;

if (!o->switchm6)
mantiuk06 = state->nothing06;
  gegl_node_link_many (state->input, state->src, state->emboss, state->rgbclip,  state->crop, state->over, state->softlight, state->multiply, state->output, NULL);
  gegl_node_connect (state->src, "aux", state->cellnoise, "output");
  gegl_node_connect (state->multiply, "aux", state->color, "output");
  gegl_node_connect (state->over, "aux", state->opacity, "output");
  gegl_node_link_many (state->crop, mantiuk06, state->opacity, NULL);
  gegl_node_connect (state->softlight, "aux", state->dog, "output");
  gegl_node_link_many (state->over, state->dog, NULL);

  gegl_node_connect (state->crop, "aux", state->input, "output");

  gegl_operation_meta_redirect (operation, "scale", state->cellnoise, "scale");
  gegl_operation_meta_redirect (operation, "seed", state->cellnoise, "seed");
  gegl_operation_meta_redirect (operation, "depth", state->emboss, "depth");
  gegl_operation_meta_redirect (operation, "elevation", state->emboss, "elevation");
  gegl_operation_meta_redirect (operation, "azimuth", state->emboss, "azimuth");
  gegl_operation_meta_redirect (operation, "value", state->color, "value");
  gegl_operation_meta_redirect (operation, "tonemap", state->opacity, "value");
  gegl_operation_meta_redirect (operation, "sldog", state->softlight, "opacity");

}
 }


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);

  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "lb:rock-surface",
    "title",       _("Rock surface texture"),
    "reference-hash", "pulseeffectsworksagain",
    "description", _("Rock surface texture"
                     ""),
    "gimp:menu-path", "<Image>/Filters/Render/Fun",
    "gimp:menu-label", _("Rock surface texture..."),
    NULL);
}

#endif
