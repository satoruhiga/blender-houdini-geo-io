from . import hio

import os

import numpy as np

import bpy
import bmesh

from mathutils import Matrix
from bpy_extras.io_utils import axis_conversion
from bpy_extras.io_utils import unpack_list


def import_mesh(geo: hio.Geometry, name: str):
    me = bpy.data.meshes.new(name)

    pdata = geo._dataByType([hio.PrimitiveTypes.Poly])

    me.vertices.add(geo.getNumPoints())
    me.vertices.foreach_set("co", geo.points().flatten())

    vertex_indices = pdata["vertices"]
    loop_start = pdata["vertex_start_index"]
    loop_total = pdata["vertex_count"]
    closed = pdata["closed"]

    ###

    me.loops.add(len(vertex_indices))
    me.loops.foreach_set("vertex_index", vertex_indices)

    me.polygons.add(len(loop_start))
    me.polygons.foreach_set("loop_start", loop_start)
    me.polygons.foreach_set("loop_total", loop_total)
    
    ###

    # Vertex attributes
    for attr in geo.vertexAttribs():
        # print('vertex', attr.name(), attr.typeInfo())

        if not (attr.dataType() == hio.AttribData.Int
                or attr.dataType() == hio.AttribData.Float):
            # print("Unsupported attribute data type: ", attr.dataType())
            continue

        # Vertex normals
        if attr.name() == "N":
            me.create_normals_split()

            me.validate(clean_customdata=False)

            me.polygons.foreach_set("use_smooth", np.ones(len(me.polygons), dtype=np.bool))

            data = attr.attribValue()
            data = data.flatten()
            data *= -1

            data = tuple(zip(*(iter(data),) * 3))

            # TODO: This function is slow, need to look for other way
            me.normals_split_custom_set(data)

            me.use_auto_smooth = True
            continue

        if attr.name() == "uv":
            data = attr.attribValue()
            data = data[:, :2]
            data = data.flatten()

            uv_layer = me.uv_layers.new(name=attr.name())
            uv_layer.data.foreach_set("uv", data)
            continue

        data = attr.attribValue()
        b_type = None

        if attr.typeInfo() == hio.TypeInfo.Value:
            b_type = "FLOAT"
            key = 'value'

        elif attr.typeInfo() == hio.TypeInfo.Vector:
            b_type = "FLOAT_VECTOR"
            key = "vector"

        elif attr.typeInfo() == hio.TypeInfo.Color:
            b_type = "FLOAT_COLOR"
            key = "color"
            data = np.column_stack((data, np.ones(data.shape[0])))

        elif attr.typeInfo() == hio.TypeInfo.TextureCoord:
            b_type = "FLOAT2"
            key = "vector"

            # vec3 to vec2
            data = data[:, :2]
        else:
            print("Unsupported attribute type: ", attr.typeInfo())
            continue

        data = data.flatten()

        ma = me.attributes.new(name=attr.name(), type=b_type, domain="CORNER")
        ma.data.foreach_set(key, data)

    ###
    
    # Point attributes
    for attr in geo.pointAttribs():
        # print('point', attr.name(), attr.typeInfo())

        if not (attr.dataType() == hio.AttribData.Int
                or attr.dataType() == hio.AttribData.Float):
            # print("Unsupported attribute data type: ", attr.dataType())
            continue

        # Skip P attribute
        if attr.typeInfo() == hio.TypeInfo.Point and attr.name() == "P":
            continue
        
        # Point normals
        if attr.name() == "N":
            me.create_normals_split()
            me.validate(clean_customdata=False)
            me.polygons.foreach_set("use_smooth", np.ones(len(me.polygons), dtype=np.bool))

            data = attr.attribValue()
            data = data.flatten()
            data *= -1

            data = tuple(zip(*(iter(data),) * 3))
            me.normals_split_custom_set_from_vertices(data)
            me.use_auto_smooth = True
            continue

        data = attr.attribValue()
        b_type = None

        if attr.typeInfo() == hio.TypeInfo.Value:
            b_type = "FLOAT"
            key = 'value'

        elif attr.typeInfo() == hio.TypeInfo.Vector:
            b_type = "FLOAT_VECTOR"
            key = "vector"

        elif attr.typeInfo() == hio.TypeInfo.Color:
            b_type = "FLOAT_COLOR"
            key = "color"
            data = np.column_stack((data, np.ones(data.shape[0])))

        elif attr.typeInfo() == hio.TypeInfo.TextureCoord:
            b_type = "FLOAT2"
            key = "vector"

            # vec3 to vec2
            data = data[:, :2]
        else:
            print("Unsupported attribute type: ", attr.typeInfo())
            continue

        data = data.flatten()

        ma = me.attributes.new(name=attr.name(), type=b_type, domain="POINT")
        ma.data.foreach_set(key, data)

    ###
    
    # Prim attributes
    for attr in geo.primAttribs():
        # print('prim', attr.name(), attr.typeInfo(), attr.dataType())

        if not (attr.dataType() == hio.AttribData.Int
                or attr.dataType() == hio.AttribData.Float):
            # print("Unsupported attribute data type: ", attr.dataType())
            continue

        if attr.name() == "material_index":
            data = attr.attribValue()
            data = data.flatten()
            me.polygons.foreach_set("material_index", data)
            continue
        
        data = attr.attribValue()
        b_type = None

        if attr.typeInfo() == hio.TypeInfo.Value:
            b_type = "FLOAT"
            key = 'value'

        elif attr.typeInfo() == hio.TypeInfo.Vector:
            b_type = "FLOAT_VECTOR"
            key = "vector"

        elif attr.typeInfo() == hio.TypeInfo.Color:
            b_type = "FLOAT_COLOR"
            key = "color"
            data = np.column_stack((data, np.ones(data.shape[0])))

        else:
            print("Unsupported attribute type: ", attr.typeInfo())
            continue

        data = data.flatten()

        ma = me.attributes.new(name=attr.name(), type=b_type, domain="FACE")
        ma.data.foreach_set(key, data)

    ###
    
    me.flip_normals()
    me.update()

    return me


def import_curve(geo: hio.Geometry, name: str):
    cu = bpy.data.curves.new(name, type="CURVE")
    cu.dimensions = "3D"
    cu.fill_mode = "FULL"

    target_type = [hio.PrimitiveTypes.NURBSCurve, hio.PrimitiveTypes.BezierCurve]
    pdata = geo._dataByType(target_type)

    points = geo.points()

    vertex_indices = pdata["vertices"]
    prim_types = pdata["type"]
    vertex_count = pdata["vertex_count"]
    vertex_start_index = pdata["vertex_start_index"]
    closed = pdata["closed"]

    for i, prim_type in enumerate(prim_types):
        t = hio.PrimitiveTypes(prim_type)

        if t == hio.PrimitiveTypes.NURBSCurve:
            sp = cu.splines.new("NURBS")
            
            N = vertex_count[i]
            sp.points.add(N - 1)

            idxs = list(range(vertex_start_index[i], vertex_start_index[i] + N))
            idxs = np.take(vertex_indices, idxs)

            pos = np.take(points, idxs, axis=0)
            pos = np.column_stack((pos, np.ones((N, 1))))
            pos = pos.flatten()

            sp.points.foreach_set("co", pos)
            sp.use_cyclic_u = closed[i]
            sp.use_endpoint_u = not closed[i]
            sp.order_u = 4

        elif t == hio.PrimitiveTypes.BezierCurve:
            sp = cu.splines.new("BEZIER")

            N = int((vertex_count[i] + 2) / 3)
            sp.bezier_points.add(N - 1)

            idxs = list(
                range(vertex_start_index[i], vertex_start_index[i] + vertex_count[i])
            )
            idxs = np.take(vertex_indices, idxs)

            pos = np.take(points, idxs, axis=0)

            if closed[i]:
                for n in range(N):
                    pt = sp.bezier_points[n]
                    pt.handle_left = pos[n * 3 - 1]
                    pt.co = pos[n * 3]
                    pt.handle_right = pos[n * 3 + 1]

            else:
                for n in range(N):
                    pt = sp.bezier_points[n]
                    if n == 0:
                        pt.handle_left = pos[0]
                        pt.co = pos[0]
                        pt.handle_right = pos[1]
                    elif n == N - 1:
                        pt.handle_left = pos[n * 3 - 1]
                        pt.co = pos[n * 3]
                        pt.handle_right = pos[n * 3]
                    else:
                        pt.handle_left = pos[n * 3 - 1]
                        pt.co = pos[n * 3]
                        pt.handle_right = pos[n * 3 + 1]

            sp.use_cyclic_u = closed[i]
            sp.use_endpoint_u = True
            sp.order_u = 4

    return cu


def import_(path: str, ob, opts):
    data = None

    temp_name = "temp_" + os.path.basename(path)

    geo = hio.Geometry()
    if not geo.load(path):
        return None

    if ob.type == "MESH":
        data = import_mesh(geo, temp_name)
        for x in ob.data.materials:
            data.materials.append(x)

    elif ob.type == 'CURVE':
        data = import_curve(geo, temp_name)

    if hasattr(data, "transform"):
        global_matrix = (
            Matrix.Scale(1, 4)
            @ axis_conversion(from_forward="-Z", from_up="Y").to_4x4()
        )
        data.transform(global_matrix)

    return data
