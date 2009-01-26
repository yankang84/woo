/*************************************************************************
*  Copyright (C) 2008 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/GLDrawFunctors.hpp>

class Ef1_BssSnowGrain_glDraw : public GLDrawInteractingGeometryFunctor
{
	public :
		virtual void go(const shared_ptr<InteractingGeometry>&, const shared_ptr<PhysicalParameters>&,bool);

	RENDERS(BssSnowGrain);
	REGISTER_CLASS_NAME(Ef1_BssSnowGrain_glDraw);
	REGISTER_BASE_CLASS_NAME(GLDrawInteractingGeometryFunctor);
};

REGISTER_SERIALIZABLE(Ef1_BssSnowGrain_glDraw);

