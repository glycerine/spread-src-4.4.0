use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'		=> 'Ensemble',
    'VERSION_FROM'	=> 'lib/Ensemble.pm', # finds $VERSION
    'LIBS'		=> ['-L../../lib -lens'], # e.g., '-lm'
    'DEFINE'		=> '', # e.g., '-DHAVE_SOMETHING'
    'INC'		=> '-I../..', # e.g., '-I/usr/include/other'
    'OBJECT'            => '$(BASEEXT)$(OBJ_EXT)'
);
