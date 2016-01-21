// Copyright (c) 2007-2016  INRIA (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Laurent Saboret, Nader Salman, Gael Guennebaud

#ifndef CGAL_POINT_SET_3_H
#define CGAL_POINT_SET_3_H

#include <CGAL/property_map.h>
#include <CGAL/Min_sphere_of_spheres_d.h>
#include <CGAL/Min_sphere_of_points_d_traits_3.h>
#include <CGAL/Min_sphere_of_spheres_d_traits_3.h>
#include <CGAL/Surface_mesh.h>

#include <algorithm>
#include <vector>
# include <CGAL/gl.h>

/// The Point_set_3 class is array of points + normals of type
/// Point_with_normal_3<Gt> (in fact
/// UI_point_3 to support a selection flag and an optional radius).
/// It provides:
/// - accessors: points and normals iterators, property maps
/// - OpenGL rendering
/// - bounding box
///
/// CAUTION:
/// - User is responsible to call invalidate_bounds() after adding, moving or removing points.
/// - Selecting points changes the order of the points in the
///   container. If selection is *not* empty, it becomes invalid after
///   adding, moving or removing points, user is reponsible to call
///   unselect_all() in those cases.
///
/// @heading Parameters:
/// @param Gt       Geometric traits class.

namespace CGAL {

template <class Gt>
class Point_set_3
{
public:
  
  typedef typename Gt::FT FT;
  typedef typename Gt::Point_3 Point;
  typedef typename Gt::Vector_3 Vector;
  typedef CGAL::cpp11::array<FT, 3> Color;
  typedef typename Gt::Iso_cuboid_3 Iso_cuboid;
  typedef typename Gt::Sphere_3 Sphere;

  
  typedef CGAL::Surface_mesh<Point> Base;
  typedef typename Base::Vertex_index Index;
  typedef typename Base::template Property_map<Index, Point> Point_pmap;
  typedef typename Base::template Property_map<Index, Vector> Vector_pmap;
  typedef typename Base::template Property_map<Index, Color> Color_pmap;

  typedef typename Base::Vertex_range::iterator iterator;
  typedef typename Base::Vertex_range::const_iterator const_iterator;

  struct build_from_pair
  {
    Point_set_3<Gt>& m_pts;

    build_from_pair (Point_set_3<Gt>& pts) : m_pts (pts) { }

    void operator() (const std::pair<typename Point_set_3<Gt>::Point,
                     typename Point_set_3<Gt>::Vector>& pair)
    {
      m_pts.push_back (pair.first, pair.second);
    }

  };

  
private:
  
  Base m_base;
  Vector_pmap m_normals;
  Color_pmap m_colors;

  // Indicate if m_barycenter, m_bounding_box, m_bounding_sphere and
  // m_diameter_standard_deviation below are valid.
  mutable bool m_bounding_box_is_valid;

  mutable Iso_cuboid m_bounding_box; // point set's bounding box
  mutable Sphere m_bounding_sphere; // point set's bounding sphere
  mutable Point m_barycenter; // point set's barycenter
  mutable FT m_diameter_standard_deviation; // point set's standard deviation

  std::size_t m_nb_selected; // handle selection

  bool m_radii_are_uptodate;

  // Assignment operator not implemented and declared private to make
  // sure nobody uses the default one without knowing it
  Point_set_3& operator= (const Point_set_3&)
  {
    return *this;
  }

  
public:

  Point_set_3 () : m_base()
  {
    m_nb_selected = 0;
    m_bounding_box_is_valid = false;
    m_radii_are_uptodate = false;
  }

  // copy constructor 
  Point_set_3 (const Point_set_3& p) : m_base(p.m_base)
  {
    m_bounding_box_is_valid = p.m_bounding_box_is_valid;
    m_bounding_box = p.m_bounding_box;
    m_barycenter = p.m_barycenter;
    m_diameter_standard_deviation = p.m_diameter_standard_deviation;

    m_nb_selected = p.nb_selected_points ();
    
    m_radii_are_uptodate = p.m_radii_are_uptodate;
  }


  Base& surface_mesh()
  {
    return m_base;
  }


  Point_pmap points()
  {
    return m_base.points();
  }


  Vector_pmap normals()
  {
    return m_normals;
  }


  void push_back (const Point& p)
  {
    m_base.add_vertex (p);
  }

  void push_back (const Point& p, const Vector& n)
  {
    Index i = m_base.add_vertex (p);
    if (!has_normals())
      {
        std::cerr << "Warning: trying to affect normal to point set without normal attributes" << std::endl;
        return;
      }
    normal(i) = n;
  }

  iterator begin() { return m_base.vertices().begin(); }
  iterator end() { return m_base.vertices().end(); }
  const_iterator begin() const { return m_base.vertices().begin(); }
  const_iterator end() const { return m_base.vertices().end(); }
  bool empty() const { return m_base.is_empty(); }
  std::size_t size () const { return m_base.number_of_vertices(); }
  void clear() { m_base.clear(); }
  Point& operator[] (std::size_t index) { return m_base.point (Index (index)); }
  const Point& operator[] (std::size_t index) const { return (*this)[index]; }

  iterator first_selected() { return end() - m_nb_selected; }
  const_iterator first_selected() const { return end() - m_nb_selected; }
  void set_first_selected(iterator it)
  {
    m_nb_selected = static_cast<std::size_t>(std::distance (it, end()));
  }
  
  // Test if point is selected
  bool is_selected(const_iterator it) const
  {
    return static_cast<std::size_t>(std::distance (it, end())) <= m_nb_selected;
  }

  /// Gets the number of selected points.
  std::size_t nb_selected_points() const
  {
    return m_nb_selected;
  }

  /// Mark a point as selected/not selected.
  void select(iterator it, bool selected = true)
  {
    bool currently = is_selected (it);
    iterator first = first_selected();
    if (currently && !selected)
      {
        std::swap (*it, *first);
        -- m_nb_selected;
      }
    else if (!currently && selected)
      {
        std::swap (*it, *first);
        ++ m_nb_selected;
      }
  }

  void select_all()
  {
    m_nb_selected = size ();
  }
  void unselect_all()
  {
    m_nb_selected = 0;
  }

  // Invert selection
  void invert_selection()
  {
    iterator sel = end() - 1;
    iterator unsel = begin();

    iterator first = first_selected();

    while (sel != first - 1 && unsel != first)
      std::swap (*(sel --), *(unsel ++));
    
    m_nb_selected = size() - m_nb_selected;
  }

  /// Deletes selected points.
  void delete_selection()
  {
    // Deletes selected points using erase-remove idiom
    for (std::size_t i = size() - 1; i != 0; -- i)
      m_base.remove_vertex (Index (i));

    m_nb_selected = 0;
    invalidate_bounds();
  }

    /// Gets the bounding box.
  Iso_cuboid bounding_box() const
  {
    if (!m_bounding_box_is_valid)
      update_bounds();

    return m_bounding_box;
  }

  /// Gets bounding sphere.
  Sphere bounding_sphere() const
  {
    if (!m_bounding_box_is_valid)
      update_bounds();

    return m_bounding_sphere;
  }

  /// Gets points barycenter.
  Point barycenter() const
  {
    if (!m_bounding_box_is_valid)
      update_bounds();

    return m_barycenter;
  }

  /// Gets the standard deviation of the distance to barycenter.
  FT diameter_standard_deviation() const
  {
    if (!m_bounding_box_is_valid)
      update_bounds();

    return m_diameter_standard_deviation;
  }

  // Gets the region of interest, ignoring the outliers.
  // This method is used to define the OpenGL arcball sphere.
  Sphere region_of_interest() const
  {
    if (!m_bounding_box_is_valid)
      update_bounds();

    // A good candidate is a sphere containing the dense region of the point cloud:
    // - center point is barycenter
    // - Radius is 2 * standard deviation
    float radius = 2.f * (float)m_diameter_standard_deviation;
    return Sphere(m_barycenter, radius*radius);
  }

  /// Update barycenter, bounding box, bounding sphere and standard deviation.
  /// User is responsible to call invalidate_bounds() after adding, moving or removing points.
  void invalidate_bounds()
  {
    m_bounding_box_is_valid = false;
  }
  
  bool are_radii_uptodate() const { return m_radii_are_uptodate; }
  void set_radii_uptodate(bool /*on*/) { m_radii_are_uptodate = false; }
  
  bool has_normals() const
  {
    std::pair<Vector_pmap, bool> pm = m_base.template property_map<Index, Vector> ("normal");
    return pm.second;
  }
  bool add_normal_property()
  {
    bool out = false;
    boost::tie (m_normals, out) = m_base.template add_property_map<Index, Vector> ("normal");
    return out;
  }
  void remove_normal_property()
  {
    m_base.remove_property_map (m_normals);
  }
  Vector& normal (std::size_t index) { return m_normals[Index (index)]; }
  const Vector& normal (std::size_t index) const { return this->normal(index); }

  bool has_colors() const
  {
    std::pair<Color_pmap, bool> pm = m_base.template property_map<Index, Color> ("color");
    return pm.second;
  }
  bool add_color_property()
  {
    bool out = false;
    boost::tie (m_colors, out) = m_base.template add_property_map<Index, Color> ("color");
    return out;
  }
  void remove_color_property()
  {
    m_base.remove_property_map (m_colors);
  }
  Color& color (std::size_t index) { return m_colors[Index (index)]; }
  const Color& color (std::size_t index) const { return this->color(index); }

  static CGAL::Second_of_pair_property_map<std::pair<Point, Vector> >
  point_property_map ()
  {
    return CGAL::make_second_of_pair_property_map (std::pair<Point, Vector>());
  }
  static CGAL::First_of_pair_property_map<std::pair<Point, Vector> >
  normal_property_map ()
  {
    return CGAL::make_first_of_pair_property_map (std::pair<Point, Vector>());
  }
  

private:

  /// Recompute barycenter, bounding box, bounding sphere and standard deviation.
  void update_bounds() const
  {
    if (begin() == end())
      return;

    // Update bounding box and barycenter.
    // TODO: we should use the functions in PCA component instead.
    FT xmin,xmax,ymin,ymax,zmin,zmax;
    xmin = ymin = zmin =  1e38;
    xmax = ymax = zmax = -1e38;
    Vector v = CGAL::NULL_VECTOR;
    FT norm = 0;
    for (const_iterator it = begin(); it != end(); it++)
    {
      const Point& p = (*this)[*it];
      

      // update bbox
      xmin = (std::min)(p.x(),xmin);
      ymin = (std::min)(p.y(),ymin);
      zmin = (std::min)(p.z(),zmin);
      xmax = (std::max)(p.x(),xmax);
      ymax = (std::max)(p.y(),ymax);
      zmax = (std::max)(p.z(),zmax);

      // update barycenter
      v = v + (p - CGAL::ORIGIN);
      norm += 1;
    }
    //
    Point p(xmin,ymin,zmin);
    Point q(xmax,ymax,zmax);
    m_bounding_box = Iso_cuboid(p,q);
    //
    m_barycenter = CGAL::ORIGIN + v / norm;

    // Computes bounding sphere
    typedef CGAL::Min_sphere_of_points_d_traits_3<Gt,FT> Traits;
    typedef CGAL::Min_sphere_of_spheres_d<Traits> Min_sphere;

    Min_sphere ms(begin(),end());

    typename Min_sphere::Cartesian_const_iterator coord = ms.center_cartesian_begin();
    FT cx = *coord++;
    FT cy = *coord++;
    FT cz = *coord++;
    m_bounding_sphere = Sphere(Point(cx,cy,cz), ms.radius()*ms.radius());

    // Computes standard deviation of the distance to barycenter
    typename Gt::Compute_squared_distance_3 sqd;
    FT sq_radius = 0;
    for (const_iterator it = begin(); it != end(); it++)
      sq_radius += sqd((*this)[*it], m_barycenter);
    sq_radius /= FT(size());
    m_diameter_standard_deviation = CGAL::sqrt(sq_radius);

    m_bounding_box_is_valid = true;
  }
  
}; // end of class Point_set_3

} // namespace CGAL

#endif // CGAL_POINT_SET_3_H
