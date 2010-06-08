/*************************************************************************
 Copyright (C) 2008 by Bruno Chareyre		                         *
*  bruno.chareyre@hmg.inpg.fr      					 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include"NewtonIntegrator.hpp"
#include<yade/core/Scene.hpp>
#include<yade/pkg-dem/Clump.hpp>
#include<yade/pkg-common/VelocityBins.hpp>
#include<yade/lib-base/Math.hpp>
		
YADE_PLUGIN((NewtonIntegrator));
CREATE_LOGGER(NewtonIntegrator);
void NewtonIntegrator::cundallDamp(const Real& dt, const Vector3r& N, const Vector3r& V, Vector3r& A){
	for(int i=0; i<3; i++) A[i]*= 1 - damping*Mathr::Sign ( N[i]*(V[i] + (Real) 0.5 *dt*A[i]) );
}
void NewtonIntegrator::blockTranslateDOFs(unsigned blockedDOFs, Vector3r& v) {
	if(blockedDOFs==State::DOF_NONE) return;
	if(blockedDOFs==State::DOF_ALL) { v=Vector3r::Zero(); return; }
	if((blockedDOFs & State::DOF_X)!=0) v[0]=0;
	if((blockedDOFs & State::DOF_Y)!=0) v[1]=0;
	if((blockedDOFs & State::DOF_Z)!=0) v[2]=0;
}
void NewtonIntegrator::blockRotateDOFs(unsigned blockedDOFs, Vector3r& v) {
	if(blockedDOFs==State::DOF_NONE) return;
	if(blockedDOFs==State::DOF_ALL) { v=Vector3r::Zero(); return; }
	if((blockedDOFs & State::DOF_RX)!=0) v[0]=0;
	if((blockedDOFs & State::DOF_RY)!=0) v[1]=0;
	if((blockedDOFs & State::DOF_RZ)!=0) v[2]=0;
}
void NewtonIntegrator::handleClumpMemberAccel(Scene* scene, const body_id_t& memberId, State* memberState, State* clumpState){
	const Vector3r& f=scene->forces.getForce(memberId);
	Vector3r diffClumpAccel=f/clumpState->mass;
	// damp increment of accels on the clump, using velocities of the clump MEMBER
	cundallDamp(scene->dt,f,memberState->vel,diffClumpAccel);
	clumpState->accel+=diffClumpAccel;
}
void NewtonIntegrator::handleClumpMemberAngAccel(Scene* scene, const body_id_t& memberId, State* memberState, State* clumpState){
	// angular acceleration from: normal torque + torque generated by the force WRT particle centroid on the clump centroid
	const Vector3r& m=scene->forces.getTorque(memberId); const Vector3r& f=scene->forces.getForce(memberId);
	Vector3r diffClumpAngularAccel=m.cwise()/clumpState->inertia+(memberState->pos-clumpState->pos).cross(f).cwise()/clumpState->inertia; 
	// damp increment of accels on the clump, using velocities of the clump MEMBER
	cundallDamp(scene->dt,m,memberState->angVel,diffClumpAngularAccel);
	clumpState->angAccel+=diffClumpAngularAccel;
}
void NewtonIntegrator::handleClumpMemberTorque(Scene* scene, const body_id_t& memberId, State* memberState, State* clumpState, Vector3r& M){
	const Vector3r& m=scene->forces.getTorque(memberId); const Vector3r& f=scene->forces.getForce(memberId);
	M+=(memberState->pos-clumpState->pos).cross(f)+m;
}
void NewtonIntegrator::saveMaximaVelocity(Scene* scene, const body_id_t& id, State* state){
	if(haveBins) velocityBins->binVelSqUse(id,VelocityBins::getBodyVelSq(state));
	#ifdef YADE_OPENMP
		Real& thrMaxVSq=threadMaxVelocitySq[omp_get_thread_num()]; thrMaxVSq=max(thrMaxVSq,state->vel.squaredNorm());
	#else
		maxVelocitySq=max(maxVelocitySq,state->vel.squaredNorm());
	#endif
}

void NewtonIntegrator::action()
{
	scene->forces.sync();
	Real dt=scene->dt;
	// account for motion of the periodic boundary, if we remember its last position
	// its velocity will count as max velocity of bodies
	// otherwise the collider might not run if only the cell were changing without any particle motion
	if(scene->isPeriodic && (prevCellSize!=scene->cell->getSize())){ cellChanged=true; maxVelocitySq=(prevCellSize-scene->cell->getSize()).squaredNorm()/pow(dt,2); }
	else { maxVelocitySq=0; cellChanged=false; }
	haveBins=(bool)velocityBins;
	if(haveBins) velocityBins->binVelSqInitialize(maxVelocitySq);

	// setup callbacks
	vector<BodyCallback::FuncPtr> callbackPtrs;
	FOREACH(const shared_ptr<BodyCallback>& cb, callbacks){
		cerr<<"<cb="<<cb.get()<<", setting cb->scene="<<scene<<">";
		cb->scene=scene;
		callbackPtrs.push_back(cb->stepInit());
	}
	assert(callbackPtrs.size()==callbacks.size());
	size_t callbacksSize=callbacks.size();


	#ifdef YADE_OPENMP
		FOREACH(Real& thrMaxVSq, threadMaxVelocitySq) { thrMaxVSq=0; }
		const BodyContainer& bodies=*(scene->bodies.get());
		const long size=(long)bodies.size();
		#pragma omp parallel for schedule(static)
		for(long _id=0; _id<size; _id++){
			const shared_ptr<Body>& b(bodies[_id]);
	#else
		FOREACH(const shared_ptr<Body>& b, *scene->bodies){
	#endif
			if(!b) continue;
			State* state=b->state.get();
			const body_id_t& id=b->getId();
			// clump members are non-dynamic; we only get their velocities here
			if (!b->isDynamic() || b->isClumpMember()){
				saveMaximaVelocity(scene,id,state);
				continue;
			}

			if (b->isStandalone()){
				// translate equation
				const Vector3r& f=scene->forces.getForce(id); 
				state->accel=f/state->mass; 
				if (!scene->isPeriodic || homotheticCellResize==0)
					cundallDamp(dt,f,state->vel,state->accel);
				else {//Apply damping on velocity fluctuations only rather than true velocity meanfield+fluctuation.
					Vector3r velFluctuation(state->vel-prevVelGrad*state->pos);
					cundallDamp(dt,f,velFluctuation,state->accel);}
				leapfrogTranslate(scene,state,id,dt);
				// rotate equation: exactAsphericalRot is disabled or the body is spherical
				if (!exactAsphericalRot || (state->inertia[0]==state->inertia[1] && state->inertia[1]==state->inertia[2])){
					const Vector3r& m=scene->forces.getTorque(id); 
					state->angAccel=m.cwise()/state->inertia;
					cundallDamp(dt,m,state->angVel,state->angAccel);
					leapfrogSphericalRotate(scene,state,id,dt);
				} else { // exactAsphericalRot enabled & aspherical body
					const Vector3r& m=scene->forces.getTorque(id); 
					leapfrogAsphericalRotate(scene,state,id,dt,m);
				}
			} else if (b->isClump()){
				// clump mass forces
				const Vector3r& f=scene->forces.getForce(id);
				Vector3r dLinAccel=f/state->mass;
				cundallDamp(dt,f,state->vel,dLinAccel);
				state->accel+=dLinAccel;
				const Vector3r& m=scene->forces.getTorque(id);
				Vector3r M(m);
				// sum force on clump memebrs
				// exactAsphericalRot enabled and clump is aspherical
				if (exactAsphericalRot && ((state->inertia[0]!=state->inertia[1] || state->inertia[1]!=state->inertia[2]))){
					FOREACH(Clump::memberMap::value_type mm, static_cast<Clump*>(b.get())->members){
						const body_id_t& memberId=mm.first;
						const shared_ptr<Body>& member=Body::byId(memberId,scene); assert(member->isClumpMember());
						State* memberState=member->state.get();
						handleClumpMemberAccel(scene,memberId,memberState,state);
						handleClumpMemberTorque(scene,memberId,memberState,state,M);
						saveMaximaVelocity(scene,memberId,memberState);
					}
					// motion
					leapfrogTranslate(scene,state,id,dt);
					leapfrogAsphericalRotate(scene,state,id,dt,M);
				} else { // exactAsphericalRot disabled or clump is spherical
					Vector3r dAngAccel=M.cwise()/state->inertia;
					cundallDamp(dt,M,state->angVel,dAngAccel);
					state->angAccel+=dAngAccel;
					FOREACH(Clump::memberMap::value_type mm, static_cast<Clump*>(b.get())->members){
						const body_id_t& memberId=mm.first;
						const shared_ptr<Body>& member=Body::byId(memberId,scene); assert(member->isClumpMember());
						State* memberState=member->state.get();
						handleClumpMemberAccel(scene,memberId,memberState,state);
						handleClumpMemberAngAccel(scene,memberId,memberState,state);
						saveMaximaVelocity(scene,memberId,memberState);
					}
					// motion
					leapfrogTranslate(scene,state,id,dt);
					leapfrogSphericalRotate(scene,state,id,dt);
				}
				static_cast<Clump*>(b.get())->moveMembers();
			}
			saveMaximaVelocity(scene,id,state);

			// process callbacks
			for(size_t i=0; i<callbacksSize; i++){
				cerr<<"<"<<b->id<<",cb="<<callbacks[i]<<",scene="<<callbacks[i]->scene<<">"; // <<",force="<<callbacks[i]->scene->forces.getForce(b->id)<<">";
				if(callbackPtrs[i]!=NULL) (*(callbackPtrs[i]))(callbacks[i].get(),b.get());
			}
	}
	#ifdef YADE_OPENMP
		FOREACH(const Real& thrMaxVSq, threadMaxVelocitySq) { maxVelocitySq=max(maxVelocitySq,thrMaxVSq); }
	#endif
	if(haveBins) velocityBins->binVelSqFinalize();
	if(scene->isPeriodic) { prevCellSize=scene->cell->getSize();prevVelGrad=scene->cell->velGrad; }
}

inline void NewtonIntegrator::leapfrogTranslate(Scene* scene, State* state, const body_id_t& id, const Real& dt)
{
	blockTranslateDOFs(state->blockedDOFs, state->accel);
	
	if (scene->forces.getMoveRotUsed()) state->pos+=scene->forces.getMove(id);
	if (homotheticCellResize>0) {
		// update velocity reflecting changes in the macroscopic velocity field, making the problem homothetic.
		//NOTE : if the velocity is updated before moving the body, it means the current velGrad (i.e. before integration in cell->integrateAndUpdate) will be effective for the current time-step. Is it correct? If not, this velocity update can be moved just after "state->pos += state->vel*dt", meaning the current velocity impulse will be applied at next iteration, after the contact law. (All this assuming the ordering is resetForces->integrateAndUpdate->contactLaw->PeriCompressor->NewtonsLaw. Any other might fool us.)
		//NOTE : dVel defined without wraping the coordinates means bodies out of the (0,0,0) period can move realy fast. It has to be compensated properly in the definition of relative velocities (see Ig2 functors and contact laws).
		//This is the convective term, appearing in the time derivation of Cundall/Thornton expression (dx/dt=velGrad*pos -> d²x/dt²=dvelGrad/dt+velGrad*vel), negligible in many cases but not for high speed large deformations (gaz or turbulent flow). Emulating Cundall is an option, I don't especially recommend it. I know homothetic 1 and 2 expressions tend to identical values in the limit of dense quasi-static situations. They can give slitghly different results in other cases, and I'm still not sure which one should be considered better, if any (Cundall formula is in contradiction with molecular dynamics litterature).
		if (homotheticCellResize>1) state->vel+=prevVelGrad*state->vel*dt;
		
		//In all cases, reflect macroscopic (periodic cell) acceleration in the velocity. This is the dominant term in the update in most cases
		Vector3r dVel=(scene->cell->velGrad-prevVelGrad)*/*scene->cell->wrapShearedPt(*/state->pos/*)*/;
		state->vel+=dVel;
	}
	state->vel+=dt*state->accel;
	state->pos += state->vel*dt;
}

inline void NewtonIntegrator::leapfrogSphericalRotate(Scene* scene, State* state, const body_id_t& id, const Real& dt )
{
	blockRotateDOFs(state->blockedDOFs, state->angAccel);
	state->angVel+=dt*state->angAccel;
	Vector3r axis = state->angVel;
	
	if (axis!=Vector3r::Zero()) {							//If we have an angular velocity, we make a rotation
		Real angle=axis.norm(); axis/=angle;
		Quaternionr q(AngleAxisr(angle*dt,axis));
		state->ori = q*state->ori;
	}
	
	if(scene->forces.getMoveRotUsed() && scene->forces.getRot(id)!=Vector3r::Zero()) {
		Vector3r r(scene->forces.getRot(id));
		Real norm=r.norm(); r/=norm;
		Quaternionr q(AngleAxisr(norm,r));
		state->ori=q*state->ori;
	}
	state->ori.normalize();
}

void NewtonIntegrator::leapfrogAsphericalRotate(Scene* scene, State* state, const body_id_t& id, const Real& dt, const Vector3r& M){
	Matrix3r A=state->ori.conjugate().toRotationMatrix(); // rotation matrix from global to local r.f.
	const Vector3r l_n = state->angMom + dt/2 * M; // global angular momentum at time n
	const Vector3r l_b_n = A*l_n; // local angular momentum at time n
	const Vector3r angVel_b_n = l_b_n.cwise()/state->inertia; // local angular velocity at time n
	const Quaternionr dotQ_n=DotQ(angVel_b_n,state->ori); // dQ/dt at time n
	const Quaternionr Q_half = state->ori + dt/2 * dotQ_n; // Q at time n+1/2
	state->angMom+=dt*M; // global angular momentum at time n+1/2
	const Vector3r l_b_half = A*state->angMom; // local angular momentum at time n+1/2
	Vector3r angVel_b_half = l_b_half.cwise()/state->inertia; // local angular velocity at time n+1/2
	blockRotateDOFs( state->blockedDOFs, angVel_b_half );
	const Quaternionr dotQ_half=DotQ(angVel_b_half,Q_half); // dQ/dt at time n+1/2
	state->ori=state->ori+dt*dotQ_half; // Q at time n+1
	state->angVel=state->ori*angVel_b_half; // global angular velocity at time n+1/2

	if(scene->forces.getMoveRotUsed() && scene->forces.getRot(id)!=Vector3r::Zero()) {
		Vector3r r(scene->forces.getRot(id));
		Real norm=r.norm(); r/=norm;
		Quaternionr q(AngleAxisr(norm,r));
		state->ori=q*state->ori;
	}
	state->ori.normalize(); 
}
// http://www.euclideanspace.com/physics/kinematics/angularvelocity/QuaternionDifferentiation2.pdf
Quaternionr NewtonIntegrator::DotQ(const Vector3r& angVel, const Quaternionr& Q){
	// FIXME: uses index access which has different meaning in Wm3 and Eigen
	Quaternionr dotQ;
	dotQ.w() = (-Q.x()*angVel[0]-Q.y()*angVel[1]-Q.z()*angVel[2])/2;
	dotQ.x() = ( Q.w()*angVel[0]-Q.z()*angVel[1]+Q.y()*angVel[2])/2;
	dotQ.y() = ( Q.z()*angVel[0]+Q.w()*angVel[1]-Q.x()*angVel[2])/2;
	dotQ.z() = (-Q.y()*angVel[0]+Q.x()*angVel[1]+Q.w()*angVel[2])/2;
	return dotQ;
}
