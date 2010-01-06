/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/GLDrawFunctors.hpp>

class Gl1_Sphere : public GlShapeFunctor{	
	private :
		static bool wire, stripes, glutNormalize;
		static int glutSlices, glutStacks;

		// for stripes
		static vector<Vector3r> vertices, faces;
		static int glSphereList;
		void subdivideTriangle(Vector3r& v1,Vector3r& v2,Vector3r& v3, int depth);
		void drawSphere(void);
		void initGlLists(void);
	public :
		virtual void go(const shared_ptr<Shape>&, const shared_ptr<State>&,bool,const GLViewInfo&);
	RENDERS(Sphere);
	REGISTER_ATTRIBUTES(Serializable,(wire)(glutNormalize)(glutSlices)(glutStacks)(stripes));
	REGISTER_CLASS_AND_BASE(Gl1_Sphere,GlShapeFunctor);
};

REGISTER_SERIALIZABLE(Gl1_Sphere);

