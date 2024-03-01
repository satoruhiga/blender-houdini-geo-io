#include <iostream>

#ifdef _WIN32
#define ssize_t ptrdiff_t
#endif

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/numpy.h>

#include "hio.h"

using namespace hio;

namespace py = pybind11;

#define DEFINE_VECTOR_PROP(CLS, NAME) \
	.def_property(#NAME, [](const CLS& self) -> float { return self.NAME(); }, [](CLS& self, float v) { self.NAME() = v; }, py::return_value_policy::copy)


PYBIND11_MODULE(CMAKE_PYMODULE_NAME, m) {

	py::enum_<AttribType> attrtype(m, "AttribType");
	attrtype
		.value("Point", AttribType::Point)
		.value("Prim", AttribType::Prim)
		.value("Vertex", AttribType::Vertex)
		.value("Global", AttribType::Global)
		;

	py::enum_<AttribData> attrdata(m, "AttribData");
	attrdata
		.value("Float", AttribData::Float)
		.value("Int", AttribData::Int)
		.value("String", AttribData::String)
		.value("Invalid", AttribData::Invalid)
		;

	py::enum_<TypeInfo> typeinfo(m, "TypeInfo");
	typeinfo
		.value("Point", TypeInfo::Point)
		.value("Vector", TypeInfo::Vector)
		.value("Normal", TypeInfo::Normal)
		.value("Color", TypeInfo::Color)
		.value("Matrix", TypeInfo::Matrix)
		.value("Quaternion", TypeInfo::Quaternion)
		.value("TextureCoord", TypeInfo::TextureCoord)
		.value("Value", TypeInfo::Value)
		;

	py::enum_<PrimitiveTypes> primtype(m, "PrimitiveTypes");
	primtype
		.value("Poly", PrimitiveTypes::Poly)
		.value("NURBSCurve", PrimitiveTypes::NURBSCurve)
		.value("BezierCurve", PrimitiveTypes::BezierCurve)
		;

	py::class_<Vector2> vector2(m, "Vector2");
	vector2
		.def(py::init<float, float>(), py::arg("x") = 0, py::arg("y") = 0)
		DEFINE_VECTOR_PROP(Vector2, x)
		DEFINE_VECTOR_PROP(Vector2, y)
		;

	py::class_<Vector3> vector3(m, "Vector3");
	vector3
		.def(py::init<float, float, float>(), py::arg("x") = 0, py::arg("y") = 0, py::arg("z") = 0)
		DEFINE_VECTOR_PROP(Vector3, x)
		DEFINE_VECTOR_PROP(Vector3, y)
		DEFINE_VECTOR_PROP(Vector3, z)
		;

	py::class_<Vector4> vector4(m, "Vector4");
	vector4
		.def(py::init<float, float, float, float>(), py::arg("x") = 0, py::arg("y") = 0, py::arg("z") = 0, py::arg("w") = 0)
		DEFINE_VECTOR_PROP(Vector4, x)
		DEFINE_VECTOR_PROP(Vector4, y)
		DEFINE_VECTOR_PROP(Vector4, z)
		DEFINE_VECTOR_PROP(Vector4, w)
		;

	py::class_<Attrib> attr(m, "Attrib");
	attr
		.def(py::init<>())
		.def("name", &Attrib::name)

		.def("size", &Attrib::size)
		.def("tupleSize", &Attrib::tupleSize)

		.def("type", &Attrib::type)
		.def("dataType", &Attrib::dataType)
		.def("typeInfo", &Attrib::typeInfo)

		.def("attribValue", [](const Attrib& self, Index offset = 0, Size size = -1) -> py::object
		{
			if (size < 0)
				size = self.size() - offset;

			if (self.dataType() == AttribData::Float)
			{
				py::array_t<float> arr(std::vector<Size>{ size, self.tupleSize() });
				self.attribValue<float>(arr.mutable_data(), offset, size);
				return arr;
			}
			else if (self.dataType() == AttribData::Int)
			{
				py::array_t<int> arr(std::vector<Size>{ size, self.tupleSize() });
				self.attribValue<int>(arr.mutable_data(), offset, size);
				return arr;
			}
			else if (self.dataType() == AttribData::String)
			{
				py::list arr;
				
				std::vector<std::string> str;
				str.resize(size);

				self.attribValue<std::string>(&str[0], offset, size);

				for (auto it : str)
					arr.append(it);
				
				return arr;
			}

			return py::none();

		}, py::arg("offset") = 0, py::arg("size") = -1)

		.def("setAttribValue", [](Attrib& self, const py::array_t<float>& data, Index offset = 0, Size size = -1) {
			auto tuple_size = data.shape()[1];

			if (self.tupleSize() != tuple_size)
				throw std::runtime_error("Tuple size mismatch");

			if (size < 0)
				size = self.size() - offset;

			self.setAttribValue<float>(data.data(), offset, size);
		}, py::arg("data"), py::arg("offset") = 0, py::arg("size") = -1)

		.def("setAttribValue", [](Attrib& self, const py::array_t<float>& data, Index offset = 0, Size size = -1) {
			auto tuple_size = data.shape()[1];

			if (self.tupleSize() != tuple_size)
				throw std::runtime_error("Tuple size mismatch");

			if (size < 0)
				size = self.size() - offset;

			self.setAttribValue<int>(data.data(), offset, size);
		}, py::arg("data"), py::arg("offset") = 0, py::arg("size") = -1)

		.def("setAttribValue", [](Attrib& self, py::list data, Index offset = 0, Size size = -1) {
			if (size < 0)
				size = self.size() - offset;

			std::vector<std::string> arr;
			for (int i = 0; i < data.size(); i++)
				arr.push_back(py::cast<std::string>(data[i]));

			self.setAttribValue<std::string>(arr.data(), offset, size);
		}, py::arg("data"), py::arg("offset") = 0, py::arg("size") = -1)

		;

	py::class_<Attrib_<float>, Attrib> float_attr(m, "FloatAttrib");
	py::class_<Attrib_<int>, Attrib> int_attr(m, "IntAttrib");
	py::class_<Attrib_<std::string>, Attrib> string_attr(m, "StringAttrib");

	py::class_<Point> point(m, "Point");
	point
		.def(py::init<Index>())
		.def("number", &Point::number)
		.def("position", &Point::position)
		.def("setPosition", &Point::setPosition)
		;

	py::class_<Vertex> vert(m, "Vertex");
	vert
		.def("number", &Vertex::number)
		;

	py::class_<Primitive> prim(m, "Primitive");
	prim
		.def("number", &Primitive::number)
		.def("vertexStartIndex", &Primitive::vertexStartIndex)
		.def("vertexCount", &Primitive::vertexCount)

		.def("vertex", &Primitive::vertex)

		.def("positions", [](Primitive& self) {
			py::array_t<float> arr(std::vector<Size>{ self.vertexCount(), 3 });
			self.positions((Vector3*)arr.mutable_data(), 0, self.vertexCount());
			return arr;
		})
		.def("setPositions", [](Primitive& self, const py::array_t<float>& data) {
			auto tuple_size = data.shape()[1];
			auto size = data.shape()[0];

			if (3 != tuple_size)
				throw std::runtime_error("Tuple size mismatch");

			if (size > self.vertexCount())
				throw std::runtime_error("Index out of range");

			self.setPositions((const Vector3*)data.data(), 0, size);
		})
		;

	py::class_<Polygon, Primitive> poly(m, "Polygon");
	poly
		.def(py::init<Primitive>())
		.def("addVertex", &Polygon::addVertex)
		.def("isClosed", &Polygon::isClosed)
		.def("setIsClosed", &Polygon::setIsClosed)
		;

	py::class_<BezierCurve, Primitive> bezier_curve(m, "BezierCurve");
	bezier_curve
		.def(py::init<Primitive>())
		.def("addVertex", &BezierCurve::addVertex)
		.def("isClosed", &BezierCurve::isClosed)
		.def("setIsClosed", &BezierCurve::setIsClosed)
		;

	py::class_<NURBSCurve, Primitive> nurbs_curve(m, "NURBSCurve");
	nurbs_curve
		.def(py::init<Primitive>())
		.def("addVertex", &NURBSCurve::addVertex)
		.def("isClosed", &NURBSCurve::isClosed)
		.def("setIsClosed", &NURBSCurve::setIsClosed)
		;

	py::class_<Geometry> geometry(m, "Geometry");
	geometry
		.def(py::init<>())
		.def("clear", &Geometry::clear)
		.def("getNumPoints", &Geometry::getNumPoints)
		.def("getNumVertices", &Geometry::getNumVertices)
		.def("getNumPrimitives", &Geometry::getNumPrimitives)

		.def("createPoint", &Geometry::createPoint)
		.def("createPoints", py::overload_cast<Size>(&Geometry::createPoints))
		.def("createPoints", [](Geometry& self, const py::array_t<float>& positions) {
			assert(positions.shape()[1] == 3);
			int N = positions.shape()[0];
			return self.createPoints(N, (const Vector3*)positions.data());
		})

		.def("point", &Geometry::point)
		.def("points", [](const Geometry& self) {
			py::array_t<float> arr(std::vector<Size>{ self.getNumPoints(), 3 });
			auto A = self.geo().getP();
			const GA_AIFTuple* tuple = A->getAIFTuple();
			tuple->getRange(A, self.geo().getPointRange(), arr.mutable_data(), 0, 3);
			return arr;
		})

		.def("prim", &Geometry::prim)
		.def("prims", &Geometry::prims)

		.def("createPolygon", &Geometry::createPolygon,
			py::arg("num_vertices") = 0, py::arg("is_closed") = true)
		.def("createPolygons", [](Geometry& self, const py::array_t<float>& positions, const py::array_t<Size>& vertex_counts, bool closed) {
			if (positions.shape()[1] != 3)
				throw std::runtime_error("`positions` shape must be (N, 3)");

			return self.createPolygons(positions.shape()[0], (const Vector3*)positions.data(),
				vertex_counts.size(), vertex_counts.data(),
				closed);
		}, py::arg("positions"), py::arg("vertex_counts"), py::arg("closed") = true)
		
		.def("createPolygons", [](Geometry& self, const py::array_t<float>& positions, const py::array_t<Index>& vertices, const py::array_t<Size>& vertex_counts, bool closed) {
			if (positions.shape()[1] != 3)
				throw std::runtime_error("`positions` shape must be (N, 3)");

			return self.createPolygons(positions.shape()[0], (const Vector3*)positions.data(),
				vertices.size(), vertices.data(),
				vertex_counts.size(), vertex_counts.data(),
				closed);
		}, py::arg("positions"), py::arg("vertices"), py::arg("vertex_counts"), py::arg("closed") = true)

		.def("createBezierCurve", &Geometry::createBezierCurve,
			py::arg("num_vertices"), py::arg("is_closed") = false, py::arg("order") = 4)
		.def("createNURBSCurve", &Geometry::createNURBSCurve,
			py::arg("num_vertices"), py::arg("is_closed") = false, py::arg("order") = 4, py::arg("_interp_ends") = -1)

		.def("pointAttribs", &Geometry::pointAttribs)
		.def("primAttribs", &Geometry::primAttribs)
		.def("vertexAttribs", &Geometry::vertexAttribs)
		.def("globalAttribs", &Geometry::globalAttribs)

		.def("addFloatAttrib", [](Geometry& self, AttribType type, const std::string& name, const std::vector<float>& default_value, TypeInfo typeinfo) {
			return self.addAttrib<float>(type, name, default_value, typeinfo);
		}, py::return_value_policy::copy)

		.def("addIntAttrib", [](Geometry& self, AttribType type, const std::string& name, const std::vector<int>& default_value, TypeInfo typeinfo) {
			return self.addAttrib<int>(type, name, default_value, typeinfo);
		}, py::return_value_policy::copy)

		.def("addStringAttrib", [](Geometry& self, AttribType type, const std::string& name, TypeInfo typeinfo) {
			return self.addAttrib<std::string>(type, name, {""}, typeinfo);
		}, py::return_value_policy::copy)
		
		.def("findPointAttrib", [](const Geometry& self, const std::string& name) -> py::object {
			auto attr = self.findPointAttrib(name);
			if (!attr) return py::none();
			return py::cast(attr);
		}, py::return_value_policy::copy)

		.def("findPrimAttrib", [](const Geometry& self, const std::string& name) -> py::object {
			auto attr = self.findPrimAttrib(name);
			if (!attr) return py::none();
			return py::cast(attr);
		}, py::return_value_policy::copy)

		.def("findVertexAttrib", [](const Geometry& self, const std::string& name) -> py::object {
			auto attr = self.findVertexAttrib(name);
			if (!attr) return py::none();
			return py::cast(attr);
		}, py::return_value_policy::copy)

		.def("findGlobalAttrib", [](const Geometry& self, const std::string& name) -> py::object {
			auto attr = self.findGlobalAttrib(name);
			if (!attr) return py::none();
			return py::cast(attr);
		}, py::return_value_policy::copy)

		.def("load", &Geometry::load)
		.def("save", &Geometry::save)

		.def("_dataByType", [](Geometry& self, std::vector<PrimitiveTypes> types) {
			{
				GA_PrimitiveGroup *grp = self.geo().newInternalPrimitiveGroup();

				static GA_PrimitiveTypeId_tag _tag;

				for (auto prim : self.prims())
				{
					bool found = false;

					PrimitiveTypes prim_type = Enum2Enum(prim.prim()->getTypeDef().getId().get(), _tag);

					for (auto t : types)
					{
						if (t == prim_type)
						{
							found = true;
							break;
						}
					}

					if (!found)
					{
						grp->addIndex(prim.number());
					}
				}

				self.geo().deletePrimitives(*grp, true);
				self.geo().destroyPrimitiveGroup(grp);
			}

			auto dict = py::dict();

			std::vector<Primitive> prims;
			prims.reserve(self.getNumPrimitives());

			for (auto prim : self.prims())
			{
				prims.push_back(prim);
			}

			py::list _prims;

			std::vector<Index> vertices;
			std::vector<Index> vertices_lut;

			std::vector<Index> vertex_start_index;
			std::vector<Size> vertex_count;

			vertices.reserve(self.getNumVertices());
			vertices_lut.reserve(self.getNumVertices());

			vertex_start_index.reserve(self.getNumPrimitives());
			vertex_count.reserve(self.getNumPrimitives());

			for (auto prim : prims)
			{
				auto vtxs = prim.vertices();
				vertices.insert(vertices.end(), vtxs.begin(), vtxs.end());
				vertex_start_index.emplace_back(prim.vertexStartIndex());
				vertex_count.emplace_back(prim.vertexCount());

				auto s = prim.vertexStartIndex();
				for (int i = 0; i < prim.vertexCount(); i++)
				{
					vertices_lut.emplace_back(s + i);
				}

				if (prim.prim()->getTypeDef().getId() == Polygon::prim_typeid)
				{
					_prims.append(Polygon(prim));
				}
			}

			dict["prims"] = _prims;

			dict["vertices"] = py::array_t<Index>(vertices.size(), vertices.data());
			dict["vertices_lut"] = py::array_t<Index>(vertices_lut.size(), vertices_lut.data());
			dict["vertex_start_index"] = py::array_t<Index>(vertex_start_index.size(), vertex_start_index.data());
			dict["vertex_count"] = py::array_t<Size>(vertex_count.size(), vertex_count.data());
						return dict;
		})
	;
}
