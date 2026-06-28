
	.global AUDIOLIB_TEST_c7xSetTCR0
AUDIOLIB_TEST_c7xSetTCR0:
	MVC	.S1	A4,		ECR785		;; Table Control Register 0
	RET	.B1

	.global AUDIOLIB_TEST_c7xSetTBR0
AUDIOLIB_TEST_c7xSetTBR0:
	MVC	.S1	A4,		ECR787		;; Table Base Register 0
	RET	.B1

	.global AUDIOLIB_TEST_c7xSetMAR
AUDIOLIB_TEST_c7xSetMAR:
	MVC	.S1	A4,		ECR789		;; Memory Attribute Register
	RET	.B1

	.global AUDIOLIB_TEST_c7xSetSCR
AUDIOLIB_TEST_c7xSetSCR:
	MVC	.S1	A4,		ECR784		;; System Control Register
	RET	.B1

   .global AUDIOLIB_TEST_c7xMmuTlbInvAll
AUDIOLIB_TEST_c7xMmuTlbInvAll:   ;; writing 0's to TLB_INV invalidates entire TLB
   ANDD    .L1     A4, 0x0, A4
   MVC     .S1     A4, TLB_INV
   RET     .B1

	.global AUDIOLIB_TEST_c7xSetL2CFG
AUDIOLIB_TEST_c7xSetL2CFG:
	MVC	.S1	A4,		ECR384		;; L2 Cache Configuration Register
	RET	.B1

	.global AUDIOLIB_TEST_c7xGetL2CFG
AUDIOLIB_TEST_c7xGetL2CFG:
	MVC	.S1	ECR384,	A4	;; L2 Cache Configuration Register
	RET	.B1

	.global AUDIOLIB_TEST_c7xSetL1DCFG
AUDIOLIB_TEST_c7xSetL1DCFG:
	MVC	.S1	A4,		ECR256		;; L1D Cache Configuration Register
	RET	.B1

	.global AUDIOLIB_TEST_c7xGetL1DCFG
AUDIOLIB_TEST_c7xGetL1DCFG:
	MVC	.S1	ECR256,	A4	;; L1D Cache Configuration Register
	RET	.B1

