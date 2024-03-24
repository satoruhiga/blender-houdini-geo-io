from email.policy import default
import os
import sys

import numpy

import bpy

from bpy.types import Panel, Menu
from bpy.types import PropertyGroup
from bpy.props import (
    BoolProperty,
    IntProperty,
    EnumProperty,
    FloatVectorProperty,
    StringProperty,
    PointerProperty,
)

import importlib

from .hio import hio

importlib.reload(hio)

from . import exporter

importlib.reload(exporter)

from . import importer

importlib.reload(importer)

bl_info = {
    "name": "Houdini Geo IO",
    "author": "satoruhiga",
    "version": (1, 0),
    "blender": (4, 0, 0),
    "location": "",
    "description": "",
    "warning": "",
    "support": "COMMUNITY",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Mesh",
}


class SCENE_OT_LoadGeo(bpy.types.Operator):
    bl_idname = "houdini_io.load_geo"
    bl_label = "NOP"
    bl_description = ""
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        ob = bpy.context.object

        o = bpy.context.object.houdini_io
        return update_geometry(o)

class SCENE_OT_SaveGeo(bpy.types.Operator):
    bl_idname = "houdini_io.save_geo"
    bl_label = "NOP"
    bl_description = ""
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        ob = bpy.context.object

        o = bpy.context.object.houdini_io
        path = o.filepath.format(name=ob.name)

        if not path:
            return {"CANCELLED"}

        bpy.ops.object.mode_set(mode="OBJECT")

        path = bpy.path.abspath(path)

        opts = {}
        res = exporter.export(path, ob, opts)

        if not res:
            return {"CANCELLED"}

        return {"FINISHED"}

###

def update_geometry(o):
    if o.load_sequence:
        try:
            path = o.filepath_template.format(name=o.name, frame=o.frame)
        except:
            print("Invalid filepath format:", o)
            return {"CANCELLED"}
    else:
        path = o.filepath

    path = bpy.path.abspath(path)

    bpy.ops.object.mode_set(mode="OBJECT")

    if not os.path.exists(path):
        print("File not found:", path)
        return {"CANCELLED"}

    ob = o.id_data
    opts = {'skip_normals': o.skip_normals and o.load_sequence}

    new_data = importer.import_(path, ob, opts)

    if not new_data:
        return {"CANCELLED"}

    name = ob.data.name

    old_data = ob.data
    old_data.name = "_temp_" + name

    new_data.name = name

    # copy materials to new data
    m = list(ob.data.materials)
    for x in m:
        new_data.materials.append(x)

    ob.data = new_data

    # remove old data
    db = eval(repr(old_data).split("[")[0])
    db.remove(old_data)

    ob.update_from_editmode()

    return {"FINISHED"}

def frame_number_changed_cb(self, context):
    update_geometry(self)

###


class SCENE_PT_HoudiniIO(Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "data"
    bl_label = "Houdini Geo IO"

    @classmethod
    def poll(cls, context):
        ob = context.object
        data = ob.data

        if (
            isinstance(data, bpy.types.Mesh)
            or isinstance(data, bpy.types.Curve)
            # or isinstance(data, bpy.types.GreasePencil)
        ):
            return True

        return False

    def draw_header(self, context):
        layout = self.layout
        layout.label(text="", icon="MESH_MONKEY")

    def draw(self, context):
        layout = self.layout

        layout.prop(bpy.context.object.houdini_io, "active", text="Active")

        if not bpy.context.object.houdini_io.active:
            return
        
        layout.separator()
        
        layout.prop(
            bpy.context.object.houdini_io, "load_sequence", text="Load Sequence"
        )

        if bpy.context.object.houdini_io.load_sequence:
            layout.prop(
                bpy.context.object.houdini_io,
                "filepath_template",
                text="Path Template {frame}",
            )
            layout.prop(bpy.context.object.houdini_io, "frame", text="Frame")

            layout.prop(bpy.context.object.houdini_io, "skip_normals", text="Skip Normals (Playback Faster)")

        else:
            layout.prop(bpy.context.object.houdini_io, "filepath", text="File")

        layout.operator(SCENE_OT_LoadGeo.bl_idname, text="Load Geo")
        layout.operator(SCENE_OT_SaveGeo.bl_idname, text="Save Geo")

###

class ObjectHoudiniIO(PropertyGroup):
    active: BoolProperty(name="Active", default=False)
    load_sequence: BoolProperty(name="Load Sequence")
    filepath_template: StringProperty(
        name="File Path Template",
        subtype="FILE_PATH",
        default="//geo/geo.{frame:04}.bgeo.sc",
    )
    frame: IntProperty(name="Frame", update=frame_number_changed_cb)

    filepath: StringProperty(
        name="File Path", subtype="FILE_PATH", default="//geo.bgeo.sc"
    )
    skip_normals: BoolProperty(name="Skip Normals", default=True)


classes = (
    ObjectHoudiniIO,
    SCENE_PT_HoudiniIO,
    SCENE_OT_LoadGeo,
    SCENE_OT_SaveGeo,
)

@bpy.app.handlers.persistent
def global_frame_change_cb(scene):
    for x in bpy.data.objects:
        hio = x.houdini_io

        if not hio.active:
            continue
        
        if hio.load_sequence:
            update_geometry(hio)

def register():
    from bpy.utils import register_class

    for cls in classes:
        register_class(cls)

    bpy.types.Object.houdini_io = PointerProperty(type=ObjectHoudiniIO)

    if not global_frame_change_cb in bpy.app.handlers.frame_change_post:
        bpy.app.handlers.frame_change_post.append(global_frame_change_cb)


def unregister():
    del bpy.types.Object.houdini_io

    from bpy.utils import unregister_class

    for cls in classes:
        unregister_class(cls)

    bpy.app.handlers.frame_change_post.remove(global_frame_change_cb)

if __name__ == "__main__":
    register()
