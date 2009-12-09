# coding: utf-8
# 2009 © Václav Šmilauer <eudoxos@arcig.cz>

"""
Functions for accessing yade's internals; only used internally.
"""
import sys
from yade import wrapper
from yade._customConverters import *
from yade import runtime
from yade import config
O=wrapper.Omega()

def childClasses(base):
	"""Recursively enumerate classes deriving from given base (as string). Returns set."""
	ret=set(O.childClassesNonrecursive(base)); ret2=set()
	for bb in ret:
		ret2|=childClasses(bb)
	return ret | ret2

_allSerializables=childClasses('Serializable')
# classes that cannot be instantiated in python directly, and will have no properties generated for them
_noPropsClasses=set(['InteractionContainer','BodyContainer','Functor','Engine','Dispatcher'])
# classes that have special wrappers; only the most-bottom ones, with their names as it is in c++
_pyRootClasses=set([
	'GlobalEngine','PartialEngine','Shape','Bound','InteractionGeometry','InteractionPhysics','FileGenerator',
	'BoundFunctor','InteractionGeometryFunctor','InteractionPhysicsFunctor','LawFunctor','Material','State']
	# childless classes
	+['BoundDispatcher','InteractionGeometryDispatcher','InteractionPhysicsDispatcher','LawDispatcher','InteractionDispatchers','ParallelEngine']
)
## set of classes for which the proxies were created
_proxiedClasses=set()

## deprecated classes
# if old class name is used, the new object is constructed and a warning is issued about old name being used
# keep chronologically ordered, oldest first; script/rename-class.py appends at the end
_deprecated={
	'CpmPhysDamageColorizer':'CpmStateUpdater', # renamed 10.10.2009
	'GLDraw_Dem3DofGeom_FacetSphere':'Gl1_Dem3DofGeom_FacetSphere', # renamed 15.11.2009
	'PeriodicInsertionSortCollider':'InsertionSortCollider',	# integrated 25.11.2009
	'BoundingVolumeMetaEngine':'BoundingDispatcher', # Tue Dec  1 14:28:29 2009, vaclav@flux  ## was BoundingVolumeDispatcher, generating double warning
	'BoundingVolumeEngineUnit':'BoundingFunctor', # Tue Dec  1 14:39:53 2009, vaclav@flux ## was BoundingVolumeFunctor, generating double warning
	'InteractionGeometryMetaEngine':'InteractionGeometryDispatcher', # Tue Dec  1 14:40:36 2009, vaclav@flux
	'InteractionPhysicsMetaEngine':'InteractionPhysicsDispatcher', # Tue Dec  1 14:40:53 2009, vaclav@flux
	'InteractionPhysicsEngineUnit':'InteractionPhysicsFunctor', # Tue Dec  1 14:41:19 2009, vaclav@flux
	'InteractionGeometryEngineUnit':'InteractionGeometryFunctor', # Tue Dec  1 14:41:56 2009, vaclav@flux
	'BoundingVolume':'Bound', # Fri Dec  4 23:44:14 2009, vaclav@flux
	'BoundingVolumeDispatcher':'BoundDispatcher', # Fri Dec  4 23:44:27 2009, vaclav@flux
	'BoundingVolumeFunctor':'BoundFunctor', # Fri Dec  4 23:44:34 2009, vaclav@flux
	'InteractingGeometry':'Shape', # Fri Dec  4 23:48:13 2009, vaclav@flux
	'ConstitutiveLaw':'LawFunctor', # Fri Dec  4 23:57:51 2009, vaclav@flux
	'ConstitutiveLawDispatcher':'LawDispatcher', # Fri Dec  4 23:57:59 2009, vaclav@flux
	'ef2_Sphere_Sphere_Dem3DofGeom':'Ig2_Sphere_Sphere_Dem3DofGeom', # Sat Dec  5 00:02:03 2009, vaclav@flux
	'ef2_Facet_Sphere_Dem3DofGeom':'Ig2_Facet_Sphere_Dem3DofGeom', # Sat Dec  5 00:02:13 2009, vaclav@flux
	'ef2_Wall_Sphere_Dem3DofGeom':'Ig2_Wall_Sphere_Dem3DofGeom', # Sat Dec  5 00:02:22 2009, vaclav@flux
	'NewtonsDampedLaw':'NewtonIntegrator', # Wed Dec  9 17:44:15 2009, vaclav@flux
	'StandAloneEngine':'GlobalEngine', # Wed Dec  9 17:46:12 2009, vaclav@flux
	'DeusExMachina':'PartialEngine', # Wed Dec  9 17:53:06 2009, vaclav@flux
	### END_RENAMED_CLASSES_LIST ### (do not delete this line; scripts/rename-class.py uses it
}


def cxxCtorsDict(proxyNamespace=__builtins__):
	"""Return dictionary of class constructors for yade's c++ types, which should be used to update a namespace.
	
	Root classes are those that are directly wrapped by boost::python. These are only put to the dict.

	Derived classes (from these root classes) are faked by creating a callable which invokes appropriate root class constructor with the derived class parameter and passes remaining arguments to it.

	Classes that are neither root nor derived are exposed via callable object that constructs a Serializable of given type and passes the parameters.
	"""
	proxyNamespace={}
	# classes derived from wrappers (but not from Serializable directly)
	for root in _pyRootClasses:
		try:
			rootType=wrapper.__dict__[root]
		except KeyError:
			print 'WARNING: class %s not defined'%root
		for p in childClasses(root):
			proxyNamespace[p]=type(p,(rootType,),{'__init__': lambda self,__subType_=p,*args,**kw: super(type(self),self).__init__(__subType_,*args,**kw)})
			_proxiedClasses.add(p)
		# inject wrapped class itself into proxyNamespace
		proxyNamespace[root]=rootType
	# classes that derive just from Serializable: the remaining ones
	for p in _allSerializables-_proxiedClasses-_pyRootClasses:
		proxyNamespace[p]=type(p,(wrapper.Serializable,),{'__init__': lambda self,__subType_=p,*args,**kw: super(type(self),self).__init__(__subType_,*args,**kw)})
	# deprecated names
	for oldName in _deprecated.keys():
		class warnWrap:
			def __init__(self,_old,_new):
				# assert(proxyNamespace.has_key(_new))
				self.old,self.new=_old,_new
			def __call__(self,*args,**kw):
				import warnings; warnings.warn("Class `%s' was renamed to (or replaced by) `%s', update your code!"%(self.old,self.new),DeprecationWarning,stacklevel=2);
				return proxyNamespace[self.new](*args,**kw)
		proxyNamespace[oldName]=warnWrap(oldName,_deprecated[oldName])
	return proxyNamespace

def setExitHandlers():
	"""Set exit handler to avoid gdb run if log4cxx crashes at exit."""
	# avoid backtrace at regular exit, even if we crash
	if 'log4cxx' in config.features:
		__builtins__['quit']=wrapper.Omega().exitNoBacktrace
		sys.exit=wrapper.Omega().exitNoBacktrace
	# this seems to be not needed anymore:
	#sys.excepthook=sys.__excepthook__ # apport on ubuntu overrides this, we don't need it

def runServers():
	"""Run python telnet server and info socket. They will be run at localhost on ports 9000 (or higher if used) and 21000 (or higer if used) respectively.
	
	The python telnet server accepts only connection from localhost,
	after authentication by random cookie, which is printed on stdout
	at server startup.

	The info socket provides read-only access to several simulation parameters
	at runtime. Each connection receives pickled dictionary with those values.
	This socket is primarily used by yade-multi batch scheduler.
	"""
	import yade.remote
	srv=yade.remote.GenericTCPServer(handler=yade.remote.PythonConsoleSocketEmulator,title='TCP python prompt',cookie=True,minPort=9000)
	yade.runtime.cookie=srv.server.cookie
	info=yade.remote.GenericTCPServer(handler=yade.remote.InfoSocketProvider,title='TCP info provider',cookie=False,minPort=21000)
	sys.stdout.flush()

