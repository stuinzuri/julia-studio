include(qtsupport_dependencies.pri)

LIBS *= -l$$qtLibraryName(QtSupport)
DEFINES += PROPARSER_AS_LIBRARY PROPARSER_THREAD_SAFE PROEVALUATOR_THREAD_SAFE