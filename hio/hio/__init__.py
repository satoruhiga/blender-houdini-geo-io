from .find_houdini import find_houdini, append_path

HFS = find_houdini()

if '18.0.566' in HFS:
	append_path(HFS['18.0.566'])
	from . import core_18_0_566 as core
else:
	raise 'No valid houdini version found: %s' % HFS.keys()

del append_path
del find_houdini
del HFS

AttribType = core.AttribType
AttribData = core.AttribData
TypeInfo = core.TypeInfo
PrimitiveTypes = core.PrimitiveTypes

Vector2 = core.Vector2
Vector3 = core.Vector3
Vector4 = core.Vector4

Attrib = core.Attrib
Point = core.Point
Vertex = core.Vertex
Polygon = core.Polygon
BezierCurve = core.BezierCurve
NURBSCurve = core.NURBSCurve
Geometry = core.Geometry

__all__ = []
