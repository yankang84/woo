
#include<yade/core/Scene.hpp>
#include<yade/pkg-common/Gl1_NormPhys.hpp>
#include<yade/pkg-common/NormShearPhys.hpp>
#include<yade/pkg-dem/DemXDofGeom.hpp>
#include<yade/pkg-dem/Shop.hpp>

YADE_PLUGIN((Gl1_NormPhys));
YADE_REQUIRE_FEATURE(OPENGL);

GLUquadric* Gl1_NormPhys::gluQuadric=NULL;
Real Gl1_NormPhys::maxFn;
Real Gl1_NormPhys::refRadius;
Real Gl1_NormPhys::maxRadius;
int Gl1_NormPhys::signFilter;
int Gl1_NormPhys::slices;
int Gl1_NormPhys::stacks;

void Gl1_NormPhys::go(const shared_ptr<InteractionPhysics>& ip, const shared_ptr<Interaction>& i, const shared_ptr<Body>& b1, const shared_ptr<Body>& b2, bool wireFrame){
	if(!gluQuadric){ gluQuadric=gluNewQuadric(); if(!gluQuadric) throw runtime_error("Gl1_NormPhys::go unable to allocate new GLUquadric object (out of memory?)."); }
	NormPhys* np=static_cast<NormPhys*>(ip.get());
	shared_ptr<InteractionGeometry> ig(i->interactionGeometry); if(!ig) return; // changed meanwhile?
	GenericSpheresContact* gsc=YADE_CAST<GenericSpheresContact*>(ig.get());
	//if(!gsc) cerr<<"Gl1_NormPhys: InteractionGeometry is not a GenericSpheresContact, but a "<<ig->getClassName()<<endl;
	Real fnNorm=np->normalForce.dot(gsc->normal);
	if((signFilter>0 && fnNorm<0) || (signFilter<0 && fnNorm>0)) return;
	int fnSign=fnNorm>0?1:-1;
	fnNorm=abs(fnNorm); 
	maxFn=max(fnNorm,maxFn);
	Real realMaxRadius;
	if(maxRadius<0){
		if(gsc->refR1>0) refRadius=min(gsc->refR1,refRadius);
		if(gsc->refR2>0) refRadius=min(gsc->refR2,refRadius);
		realMaxRadius=refRadius;
	}
	else realMaxRadius=maxRadius;
	Real radius=realMaxRadius*(fnNorm/maxFn); // use logarithmic scale here?
	Vector3r color=Shop::scalarOnColorScale(fnNorm*fnSign,-maxFn,maxFn);
	Vector3r p1=b1->state->pos, p2=b2->state->pos;
	Vector3r relPos;
	if(scene->isPeriodic){
		relPos=p2+scene->cell->Hsize*i->cellDist.cast<Real>()-p1;
		p1=scene->cell->wrapShearedPt(p1);
		p2=p1+relPos;
	} else {
		relPos=p2-p1;
	}
	Real dist=relPos.norm();

	glDisable(GL_CULL_FACE); 
	glPushMatrix();
		glTranslatef(p1[0],p1[1],p1[2]);
		Quaternionr q(Quaternionr().setFromTwoVectors(Vector3r(0,0,1),relPos/dist /* normalized */));
		// using Transform with OpenGL: http://eigen.tuxfamily.org/dox/TutorialGeometry.html
		glMultMatrixd(Eigen::Transform3d(q).data());
		glColor3v(color);
		gluCylinder(gluQuadric,radius,radius,dist,slices,stacks);
	glPopMatrix();
}
