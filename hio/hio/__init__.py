import os
from .find_houdini import find_houdini, append_path

HFS = find_houdini()

if not HFS:
	raise ImportError('Houdini not found')

_mod_list = filter(lambda x: x.startswith('core_'),
				  os.listdir(os.path.dirname(__file__)))
_mod_list = list(map(lambda x: x.split('.')[0], _mod_list))

core = None

for HOUDINI_VERSION in sorted(HFS.keys(), reverse=True):
	mod_name = 'core_%s' % HOUDINI_VERSION.replace('.', '_')

	if not mod_name in _mod_list:
		continue

	append_path(HFS[HOUDINI_VERSION])

	import importlib
	core = importlib.import_module('.' + mod_name, package=__package__)
	break

if not core:
	raise ImportError('Houdini version not supported. You should build core module for your Houdini version.')

del append_path
del find_houdini
del HFS
del _mod_list

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
