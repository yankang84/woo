#pragma once
#include<yade/pkg-dem/DemXDofGeom.hpp>
// for static roll/unroll functions in Dem3DofGeom_SphereSphere
#include<yade/pkg-dem/Dem3DofGeom_SphereSphere.hpp>

class Dem3DofGeom_WallSphere: public Dem3DofGeom{

	Vector3r contPtInTgPlane1() const { return se31.position+cp1pt-contactPoint; }
	Vector3r contPtInTgPlane2() const { return Dem3DofGeom_SphereSphere::unrollSpherePtToPlane(se32.orientation*cp2rel,effR2,-normal);}

	public:
		Dem3DofGeom_WallSphere(){ createIndex();}
		virtual ~Dem3DofGeom_WallSphere();
		/******* API ********/
		virtual Real displacementN(){ return (se32.position-contactPoint).Length()-refLength;}
		virtual Vector3r displacementT(){ relocateContactPoints(); return contPtInTgPlane2()-contPtInTgPlane1(); }
		virtual Real slipToDisplacementTMax(Real displacementTMax);
		/***** end API ******/

		void setTgPlanePts(const Vector3r&, const Vector3r&);
		void relocateContactPoints(){ relocateContactPoints(contPtInTgPlane1(),contPtInTgPlane2()); }
		void relocateContactPoints(const Vector3r& p1, const Vector3r& p2);
	//! initial contact point on the wall, relative to the current contact point
	Vector3r cp1pt;
	//! orientation between +x and the reference contact point (on the sphere) in sphere-local coords
	Quaternionr cp2rel;
	// effective radius of sphere
	Real effR2;
	REGISTER_ATTRIBUTES(Dem3DofGeom,(cp1pt)(cp2rel)(effR2));
	REGISTER_CLASS_AND_BASE(Dem3DofGeom_WallSphere,Dem3DofGeom);
	REGISTER_CLASS_INDEX(Dem3DofGeom_WallSphere,Dem3DofGeom);
	DECLARE_LOGGER;
	friend class Gl1_Dem3DofGeom_WallSphere;
	friend class ef2_Wall_Sphere_Dem3DofGeom;
};
REGISTER_SERIALIZABLE(Dem3DofGeom_WallSphere);

#ifdef YADE_OPENGL
	#include<yade/pkg-common/GLDrawFunctors.hpp>
	class Gl1_Dem3DofGeom_WallSphere:public GLDrawInteractionGeometryFunctor{
		public:
			virtual void go(const shared_ptr<InteractionGeometry>&,const shared_ptr<Interaction>&,const shared_ptr<Body>&,const shared_ptr<Body>&,bool wireFrame);
			static bool normal,rolledPoints,unrolledPoints,shear,shearLabel;
		RENDERS(Dem3DofGeom_WallSphere);
		REGISTER_CLASS_AND_BASE(Gl1_Dem3DofGeom_WallSphere,GLDrawInteractionGeometryFunctor);
		REGISTER_ATTRIBUTES(GLDrawInteractionGeometryFunctor, (normal)(rolledPoints)(unrolledPoints)(shear)(shearLabel) );
	};
	REGISTER_SERIALIZABLE(Gl1_Dem3DofGeom_WallSphere);
#endif

#include<yade/pkg-common/InteractionGeometryEngineUnit.hpp>
class ef2_Wall_Sphere_Dem3DofGeom:public InteractionGeometryEngineUnit{
	public:
		virtual bool go(const shared_ptr<InteractingGeometry>& cm1, const shared_ptr<InteractingGeometry>& cm2, const Se3r& se31, const Se3r& se32, const shared_ptr<Interaction>& c);
		virtual bool goReverse(	const shared_ptr<InteractingGeometry>& cm1, const shared_ptr<InteractingGeometry>& cm2, const Se3r& se31, const Se3r& se32, const shared_ptr<Interaction>& c){
			c->swapOrder(); return go(cm2,cm1,se32,se31,c);
			LOG_ERROR("!! goReverse might not work in ef2_Wall_Sphere_Dem3DofGeom. InteractionGeometryMetaEngine should swap interaction members first and call go(...) afterwards.");
		}
		ef2_Wall_Sphere_Dem3DofGeom(){}

	FUNCTOR2D(Wall,InteractingSphere);
	DEFINE_FUNCTOR_ORDER_2D(Wall,InteractingSphere);
	REGISTER_CLASS_AND_BASE(ef2_Wall_Sphere_Dem3DofGeom,InteractionGeometryEngineUnit);
	REGISTER_ATTRIBUTES(InteractionGeometryEngineUnit,);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(ef2_Wall_Sphere_Dem3DofGeom);
