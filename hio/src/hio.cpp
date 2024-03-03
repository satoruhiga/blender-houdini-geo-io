#include "hio.h"

#include <numeric>
#include <iostream>
#include <fstream>

namespace hio {

	Geometry::Geometry()
	{
		_geo.clearAndDestroy();
	}

	void Geometry::clear()
	{
		_geo.clearAndDestroy();
	}

    void Geometry::reverse()
	{
	    _geo.reverse();
	}

	hio::Size Geometry::getNumPoints() const
	{
		return _geo.getNumPoints();
	}

	hio::Size Geometry::getNumVertices() const
	{
		return _geo.getNumVertices();
	}

	hio::Size Geometry::getNumPrimitives() const
	{
		return _geo.getNumPrimitives();
	}

	hio::Point Geometry::createPoint()
	{
		return Point(_geo.appendPoint());
	}

	std::vector<hio::Point> Geometry::createPoints(Size size)
	{
		auto start = _geo.appendPointBlock(size);

		std::vector<hio::Point> arr;
		for (int i = 0; i < size; i++)
			arr.emplace_back(start + i);
		return arr;
	}

	std::vector<Point> Geometry::createPoints(Size size, const Vector3* data)
	{
		auto start = _geo.appendPointBlock(size);

		auto P = _geo.getP();
		auto tuple = P->getAIFTuple();
		auto res = tuple->setRange(P, _geo.getPointRange(), (float*)data, 0, 3);
		assert(res);

		std::vector<hio::Point> arr;
		for (int i = 0; i < size; i++)
			arr.emplace_back(start + i);
		return arr;
	}

	hio::Point Geometry::point(Index index) const
	{
		assert(index >= 0 && index < getNumPoints());
		return hio::Point(index);
	}

	std::vector<hio::Point> Geometry::points() const
	{
		std::vector<hio::Point> pts;
		for (int i = 0; i < getNumPoints(); i++)
			pts.emplace_back(i);
		return pts;
	}

	hio::Primitive Geometry::prim(Index index) const
	{
		assert(index >= 0 && index < getNumPrimitives());
		return Primitive((GEO_Primitive*)_geo.getPrimitiveByIndex(index));
	}

	std::vector<hio::Primitive> Geometry::prims() const
	{
		std::vector<hio::Primitive> arr;

		for (int i = 0; i < _geo.getNumPrimitives(); i++)
		{
			auto prim = _geo.getPrimitiveByIndex(i);
			arr.emplace_back((GEO_Primitive*)prim);
		}

		return std::move(arr);
	}

	hio::Polygon Geometry::createPolygon(Size num_vertices, bool is_closed)
	{
		GEO_PrimPoly* poly = (GEO_PrimPoly*)GU_PrimPoly::build(&_geo, num_vertices, !is_closed, true);
		return Polygon(poly);
	}


	std::vector<hio::Polygon> Geometry::createPolygons(Size position_size, const Vector3* positions, Size vertex_counts_size, const Size* vertex_counts, bool closed)
	{
		if (std::accumulate(vertex_counts, vertex_counts + vertex_counts_size, 0) != position_size)
			throw std::runtime_error("Position and vertex count mismatch");

		auto pts = createPoints(position_size, positions);

		auto pt_it = pts.begin();

		std::vector<hio::Polygon> arr;
		arr.reserve(vertex_counts_size);

		for (int x = 0; x < vertex_counts_size; x++)
		{
			auto count = vertex_counts[x];

			auto poly = createPolygon();
			poly.setIsClosed(closed);
			arr.push_back(poly);

			for (int i = 0; i < count; i++)
			{
				poly.addVertex(*pt_it);
				pt_it++;
			}
		}

		return arr;
	}

	std::vector<hio::Polygon> Geometry::createPolygons(Size position_size, const Vector3* positions, Size vertices_size, const Index* vertices, Size vertex_counts_size, const Size* vertex_counts, bool closed)
	{
		auto pts = createPoints(position_size, positions);

		auto vtx_it = vertices;

		std::vector<hio::Polygon> arr;
		arr.reserve(vertex_counts_size);

		for (int x = 0; x < vertex_counts_size; x++)
		{
			auto count = vertex_counts[x];

			auto poly = createPolygon();
			poly.setIsClosed(closed);
			arr.push_back(poly);

			for (int i = 0; i < count; i++)
			{
				auto pt = pts[*vtx_it];
				poly.addVertex(pt);
				vtx_it++;
			}
		}

		return arr;

	}
	
	hio::BezierCurve Geometry::createBezierCurve(Size num_vertices, bool is_closed, int order)
	{
		GEO_PrimRBezCurve* curve = GU_PrimRBezCurve::build(&_geo, num_vertices, order, is_closed, true);
		return BezierCurve(curve);
	}

	hio::NURBSCurve Geometry::createNURBSCurve(Size num_vertices, bool is_closed, int order, int _interp_ends)
	{
		int interpEnds = is_closed ? 0 : 1;
		if (_interp_ends >= 0)
			interpEnds = _interp_ends;

		GEO_PrimNURBCurve* curve = GU_PrimNURBCurve::build(&_geo, num_vertices, order, is_closed, interpEnds, true);
		return NURBSCurve(curve);
	}

	void Geometry::deletePrims(const std::vector<Primitive>& prims, bool keep_points)
	{
		GA_PrimitiveGroup *grp = geo().newInternalPrimitiveGroup();

		for (auto prim : prims)
			grp->addIndex(prim.number());

		geo().deletePrimitives(*grp, !keep_points);
		geo().destroyPrimitiveGroup(grp);
	}

	std::vector<Attrib> Geometry::pointAttribs() const
	{
		const auto& attrs = _geo.pointAttribs();
		
		std::vector<Attrib> arr;

		GA_AttributeDict::iterator it = attrs.begin(GA_SCOPE_PUBLIC);
		while (it != attrs.end())
		{
			arr.push_back(*it);
			it.operator++();
		}

		return arr;
	}

	std::vector<Attrib> Geometry::primAttribs() const
	{
		const auto& attrs = _geo.primitiveAttribs();

		std::vector<Attrib> arr;
		GA_AttributeDict::iterator it = attrs.begin(GA_SCOPE_PUBLIC);
		while (it != attrs.end())
		{
			arr.push_back(*it);
			it.operator++();
		}

		return arr;
	}

	std::vector<Attrib> Geometry::vertexAttribs() const
	{
		const auto& attrs = _geo.vertexAttribs();

		std::vector<Attrib> arr;
		GA_AttributeDict::iterator it = attrs.begin(GA_SCOPE_PUBLIC);
		while (it != attrs.end())
		{
			arr.push_back(*it);
			it.operator++();
		}

		return arr;
	}

	std::vector<Attrib> Geometry::globalAttribs() const
	{
		const auto& attrs = _geo.attribs();

		std::vector<Attrib> arr;
		GA_AttributeDict::iterator it = attrs.begin(GA_SCOPE_PUBLIC);
		while (it != attrs.end())
		{
			arr.push_back(*it);
			it.operator++();
		}

		return arr;
	}

	hio::Attrib Geometry::findPointAttrib(const std::string& name) const
	{
		return _geo.findPointAttribute(GA_SCOPE_PUBLIC, UT_StringRef(name.c_str()));
	}

	hio::Attrib Geometry::findPrimAttrib(const std::string& name) const
	{
		return _geo.findPrimitiveAttribute(GA_SCOPE_PUBLIC, UT_StringRef(name.c_str()));
	}

	hio::Attrib Geometry::findVertexAttrib(const std::string& name) const
	{
		return _geo.findVertexAttribute(GA_SCOPE_PUBLIC, UT_StringRef(name.c_str()));
	}

	hio::Attrib Geometry::findGlobalAttrib(const std::string& name) const
	{
		return _geo.findGlobalAttribute(GA_SCOPE_PUBLIC, UT_StringRef(name.c_str()));
	}
    
	///

    void Geometry::filterPrimitiveByType(std::vector<PrimitiveTypes> prim_types)
	{
	    const auto& prims = this->prims();
	    
	    auto group = _geo.createDetachedPrimitiveGroup();

	    for (const auto& prim : prims)
	    {
            auto it = std::find_if(prim_types.begin(), prim_types.end(), [&](PrimitiveTypes p)
                { return prim.getTypeID() == (int)p; });
	        
	        if (it != prim_types.end())
	        {
	            group->add(prim.prim());
	        }
	    }

	    _geo.destroyPrimitives(GA_Range(*group, true), true);
	}

	bool Geometry::load(const std::string& path)
	{
		GA_LoadOptions opts;
		UT_StringArray errors;

		std::string _path = path;
		std::replace(_path.begin(), _path.end(), '\\', '/');

		auto res = _geo.load(_path.c_str(), &opts, &errors);
		if (!res.success())
		{
			for (auto s : errors)
				std::cerr << s << std::endl;
			return false;
		}

		return true;
	}

	bool Geometry::save(const std::string& path)
	{
		UT_StringArray errors;
		GA_SaveOptions opts;

		std::string _path = path;
		std::replace(_path.begin(), _path.end(), '\\', '/');

		auto res = _geo.save(_path.c_str(), &opts, &errors);
		if (!res.success())
		{
			for (auto s : errors)
				std::cerr << s << std::endl;
			return false;
		}

		return true;
	}

	void Primitive::setPositions(const Vector3* data, Index offset, Size size)
	{
		Attrib P(prim()->getDetail().getP());
		P.setAttribValue<float>(data, offset + _prim->getVertexOffset(0), size);
	}

	void Primitive::positions(Vector3* out_data, Index offset, Size size)
	{
		Attrib P(prim()->getDetail().getP());
		P.attribValue<float>(out_data, offset + _prim->getVertexOffset(0), size);
	}

	hio::Vertex Primitive::vertex(Index index)
	{
		return Vertex(_prim->getVertexIndex(index));
	}

	hio::Vector3 Point::position(const Geometry& geo) const
	{
		return geo.geo().getPos3(index);
	}

	void Point::setPosition(Geometry& geo, const Vector3& P)
	{
		return geo.geo().setPos3(index, P);
	}

	Polygon::Polygon(GEO_Primitive* prim)
		: Primitive(prim)
	{
		if (prim->getTypeDef().getId() != Polygon::prim_typeid)
			throw std::runtime_error("Invalid cast");
	}

	Polygon::Polygon(const Primitive& cast)
		: Primitive(cast.prim())
	{
		if (cast.prim()->getTypeDef().getId() != Polygon::prim_typeid)
			throw std::runtime_error("Invalid cast");
	}

	hio::Vertex Polygon::addVertex(Point point)
	{
		auto vtx = poly()->appendVertex(point.number());
		return Vertex(vtx);
	}

	BezierCurve::BezierCurve(GEO_Primitive* prim)
		: Primitive(prim)
	{
		if (prim->getTypeDef().getId() != BezierCurve::prim_typeid)
			throw std::runtime_error("Invalid cast");
	}

	BezierCurve::BezierCurve(const Primitive& cast)
		: Primitive(cast.prim())
	{
		if (cast.prim()->getTypeDef().getId() != BezierCurve::prim_typeid)
			throw std::runtime_error("Invalid cast");
	}

	hio::Vertex BezierCurve::addVertex(Point point)
	{
		auto vtx = curve()->appendVertex(point.number());
		return Vertex(vtx);
	}

	NURBSCurve::NURBSCurve(GEO_Primitive* prim)
		: Primitive(prim)
	{
		if (prim->getTypeDef().getId() != NURBSCurve::prim_typeid)
			throw std::runtime_error("Invalid cast");
	}

	NURBSCurve::NURBSCurve(const Primitive& cast)
		: Primitive(cast.prim())
	{
		if (cast.prim()->getTypeDef().getId() != NURBSCurve::prim_typeid)
			throw std::runtime_error("Invalid cast");
	}

	hio::Vertex NURBSCurve::addVertex(Point point)
	{
		auto vtx = curve()->appendVertex(point.number());
		return Vertex(vtx);
	}

}
