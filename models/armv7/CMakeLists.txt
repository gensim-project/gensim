
define_model(
	model-armv7a 
	armv7a
	armv7a.ac

	armv5e_thumb_isa.ac
	armv7a.ac
	armv7a_isa.ac
	armv7a_thumb_isa.ac
	behaviours.arm
	behaviours.thumb1
	behaviours.thumb2
	decode.neon
	decode.vfpv4
	execute.arm
	execute.neon
	execute.thumb1
	execute.thumb2
	execute.thumb_vfp
	execute.vfpv4
	neon.ac
	thumb_vfp.ac
	vfpv4.ac
)


IF(TESTING_ENABLED)
	ADD_SUBDIRECTORY(tests)
ENDIF()
