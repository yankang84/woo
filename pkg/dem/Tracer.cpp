#include<woo/pkg/dem/Tracer.hpp>
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>

#include<woo/pkg/dem/Sphere.hpp>

WOO_PLUGIN(gl,(TraceGlRep));
WOO_PLUGIN(dem,(Tracer));

void TraceGlRep::compress(int ratio){
	int i;
	int skip=(Tracer::compSkip<0?ratio:Tracer::compSkip);
	for(i=0; ratio*i+skip<(int)pts.size(); i++){
		pts[i]=pts[ratio*i+skip];
		scalars[i]=scalars[ratio*i+skip];
	}
	writeIx=i;
}
void TraceGlRep::addPoint(const Vector3r& p, const Real& scalar){
	if(flags&FLAG_MINDIST){
		size_t lastIx=(writeIx>0?writeIx-1:pts.size());
		if((p-pts[lastIx]).norm()<Tracer::minDist) return;
	}
	pts[writeIx]=p;
	scalars[writeIx]=scalar;
	if(flags&FLAG_COMPRESS){
		if(writeIx>=pts.size()-1) compress(Tracer::compress);
		else writeIx+=1;
	} else {
		writeIx=(writeIx+1)%pts.size();
	}
}

void TraceGlRep::render(const shared_ptr<Node>& n, GLViewInfo*){
	if(isHidden()) return;
	if(!Tracer::glSmooth) glDisable(GL_LINE_SMOOTH);
	else glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_STRIP);
		for(size_t i=0; i<pts.size(); i++){
			size_t ix; Real relColor;
			// compressed start from the very beginning, till they reach the write position
			if(flags&FLAG_COMPRESS){
				ix=i;
				relColor=i*1./writeIx;
				if(ix>=writeIx) break;
			// cycle through the array
			} else {
				ix=(writeIx+i)%pts.size();
				relColor=i*1./pts.size();
			}
			if(!isnan(pts[ix][0])){
				if(isnan(scalars[ix])){
					// if there is no scalar and no scalar should be saved, color by history position
					if(Tracer::scalar==Tracer::SCALAR_NONE) glColor3v(Tracer::lineColor->color((flags&FLAG_COMPRESS ? i*1./writeIx : i*1./pts.size())));
					// if other scalars are saved, use noneColor to not destroy Tracer::lineColor range by auto-adjusting to bogus
					else glColor3v(Tracer::noneColor);
				}
				else glColor3v(Tracer::lineColor->color(scalars[ix]));
				glVertex3v(pts[ix]);
			}
		}
	glEnd();
}

void TraceGlRep::resize(size_t size){
	pts.resize(size,Vector3r(NaN,NaN,NaN));
	scalars.resize(size,NaN);
}

Vector2i Tracer::modulo;
Vector2r Tracer::rRange;
int Tracer::num;
int Tracer::scalar;
int Tracer::lastScalar;
int Tracer::compress;
int Tracer::compSkip;
bool Tracer::glSmooth;
Vector3r Tracer::noneColor;
Real Tracer::minDist;
shared_ptr<ScalarRange> Tracer::lineColor;

void Tracer::resetNodesRep(bool setupEmpty){
	auto& dem=field->cast<DemField>();
	boost::mutex::scoped_lock lock(dem.nodesMutex);
	for(const auto& p: *dem.particles){
		for(const auto& n: p->shape->nodes){
			n->rep.reset();
			if(setupEmpty){
				n->rep=make_shared<TraceGlRep>();
				auto& tr=n->rep->cast<TraceGlRep>();
				tr.resize(num);
				tr.flags=(compress>0?TraceGlRep::FLAG_COMPRESS:0) | (minDist>0?TraceGlRep::FLAG_MINDIST:0);
			}
		}
	}
}

void Tracer::run(){
	if(scalar!=lastScalar){
		resetNodesRep(/*setup empty*/true);
		lastScalar=scalar;
		lineColor->reset();
	}
	switch(scalar){
		case SCALAR_NONE: lineColor->label="[index]"; break;
		case SCALAR_TIME: lineColor->label="time"; break;
		case SCALAR_VEL: lineColor->label="|vel|"; break;
		case SCALAR_SIGNED_ACCEL: lineColor->label="signed |accel|"; break;
		case SCALAR_RADIUS: lineColor->label="radius"; break;
		case SCALAR_SHAPE_COLOR: lineColor->label="Shape.color"; break;
	}
	auto& dem=field->cast<DemField>();
	for(const auto& p: *dem.particles){
		for(const auto& n: p->shape->nodes){
			// node added
			if(!n->rep || !dynamic_pointer_cast<TraceGlRep>(n->rep)){
				boost::mutex::scoped_lock lock(dem.nodesMutex);
				n->rep=make_shared<TraceGlRep>();
				auto& tr=n->rep->cast<TraceGlRep>();
				tr.resize(num);
				tr.flags=(compress>0?TraceGlRep::FLAG_COMPRESS:0) | (minDist>0?TraceGlRep::FLAG_MINDIST:0);
			}
			auto& tr=n->rep->cast<TraceGlRep>();
			bool hidden=false;
			if(modulo[0]>0 && (p->id+modulo[1])%modulo[0]!=0) hidden=true;
			if(!hidden && rRange.maxCoeff()>0){
				if(dynamic_pointer_cast<Sphere>(p->shape)){
					Real r=p->shape->cast<Sphere>().radius;
					hidden=((rRange[0]>0 && r<rRange[0]) || (rRange[1]>0 && r>rRange[1]));
				} else {
					hidden=true; // hide traces of non-spheres
				}
			}
			if(tr.isHidden()!=hidden) tr.setHidden(hidden);
			Real sc;
			switch(scalar){
				case SCALAR_VEL: sc=n->getData<DemData>().vel.norm(); break;
				case SCALAR_SIGNED_ACCEL:{
					const auto& dyn=n->getData<DemData>();
					if(dyn.mass==0) sc=NaN;
					else sc=(dyn.vel.dot(dyn.force)>0?1:-1)*dyn.force.norm()/dyn.mass;
					break;
				}
				case SCALAR_RADIUS: sc=(dynamic_pointer_cast<Sphere>(p->shape)?p->shape->cast<Sphere>().radius:NaN); break;
				case SCALAR_SHAPE_COLOR: sc=p->shape->color; break;
				case SCALAR_TIME: sc=scene->time; break;
				default: sc=NaN;
			}
			tr.addPoint(n->pos,sc);
		}
	}
}