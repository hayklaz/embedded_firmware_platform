#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/Embedded_Firmware_Platform.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/Embedded_Firmware_Platform.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../src/ADCFilter.c ../src/BOARD.c ../src/I2C.c ../src/NonVolatileMemory.c ../src/Protocol2.c ../src/Uart.c ../src/Main.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1360937237/ADCFilter.o ${OBJECTDIR}/_ext/1360937237/BOARD.o ${OBJECTDIR}/_ext/1360937237/I2C.o ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o ${OBJECTDIR}/_ext/1360937237/Protocol2.o ${OBJECTDIR}/_ext/1360937237/Uart.o ${OBJECTDIR}/_ext/1360937237/Main.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1360937237/ADCFilter.o.d ${OBJECTDIR}/_ext/1360937237/BOARD.o.d ${OBJECTDIR}/_ext/1360937237/I2C.o.d ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o.d ${OBJECTDIR}/_ext/1360937237/Protocol2.o.d ${OBJECTDIR}/_ext/1360937237/Uart.o.d ${OBJECTDIR}/_ext/1360937237/Main.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1360937237/ADCFilter.o ${OBJECTDIR}/_ext/1360937237/BOARD.o ${OBJECTDIR}/_ext/1360937237/I2C.o ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o ${OBJECTDIR}/_ext/1360937237/Protocol2.o ${OBJECTDIR}/_ext/1360937237/Uart.o ${OBJECTDIR}/_ext/1360937237/Main.o

# Source Files
SOURCEFILES=../src/ADCFilter.c ../src/BOARD.c ../src/I2C.c ../src/NonVolatileMemory.c ../src/Protocol2.c ../src/Uart.c ../src/Main.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/Embedded_Firmware_Platform.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32MX340F512H
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1360937237/ADCFilter.o: ../src/ADCFilter.c  .generated_files/flags/default/4c8f4e3ca398c252c41b9b3c67c179ec3522e2c5 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ADCFilter.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ADCFilter.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_SIMULATOR=1  -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/ADCFilter.o.d" -o ${OBJECTDIR}/_ext/1360937237/ADCFilter.o ../src/ADCFilter.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/BOARD.o: ../src/BOARD.c  .generated_files/flags/default/5c8eef21d58b43e009545f2391302ae072ba424c .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/BOARD.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/BOARD.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_SIMULATOR=1  -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/BOARD.o.d" -o ${OBJECTDIR}/_ext/1360937237/BOARD.o ../src/BOARD.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/I2C.o: ../src/I2C.c  .generated_files/flags/default/69fd1b8056523ccb4346451096bf8740eb3476c9 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/I2C.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/I2C.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_SIMULATOR=1  -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/I2C.o.d" -o ${OBJECTDIR}/_ext/1360937237/I2C.o ../src/I2C.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o: ../src/NonVolatileMemory.c  .generated_files/flags/default/97393256af753af8e6f4a872950a632e762d3f5 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_SIMULATOR=1  -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o.d" -o ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o ../src/NonVolatileMemory.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/Protocol2.o: ../src/Protocol2.c  .generated_files/flags/default/939bb1f611ad9f4683fa3b0c2657dd0ade070582 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Protocol2.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Protocol2.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_SIMULATOR=1  -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/Protocol2.o.d" -o ${OBJECTDIR}/_ext/1360937237/Protocol2.o ../src/Protocol2.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/Uart.o: ../src/Uart.c  .generated_files/flags/default/f59a3a64bd53b6aaec53ea6d8969b496c2c26e10 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_SIMULATOR=1  -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/Uart.o.d" -o ${OBJECTDIR}/_ext/1360937237/Uart.o ../src/Uart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/Main.o: ../src/Main.c  .generated_files/flags/default/f8234d83c35b38c5e79c9a9e7ac46e0306ae0c39 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_SIMULATOR=1  -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/Main.o.d" -o ${OBJECTDIR}/_ext/1360937237/Main.o ../src/Main.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
else
${OBJECTDIR}/_ext/1360937237/ADCFilter.o: ../src/ADCFilter.c  .generated_files/flags/default/c5c25a8680eda0de411f584f25336cac7b533a62 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ADCFilter.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ADCFilter.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/ADCFilter.o.d" -o ${OBJECTDIR}/_ext/1360937237/ADCFilter.o ../src/ADCFilter.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/BOARD.o: ../src/BOARD.c  .generated_files/flags/default/77469ca11076d5fba291b81d59960171d37fb4b9 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/BOARD.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/BOARD.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/BOARD.o.d" -o ${OBJECTDIR}/_ext/1360937237/BOARD.o ../src/BOARD.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/I2C.o: ../src/I2C.c  .generated_files/flags/default/279bbc000c2801b80ede6c59ef49eaddc0e0896b .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/I2C.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/I2C.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/I2C.o.d" -o ${OBJECTDIR}/_ext/1360937237/I2C.o ../src/I2C.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o: ../src/NonVolatileMemory.c  .generated_files/flags/default/fdcb210f5b94214c422609282a4c3fb6d67d9e2 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o.d" -o ${OBJECTDIR}/_ext/1360937237/NonVolatileMemory.o ../src/NonVolatileMemory.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/Protocol2.o: ../src/Protocol2.c  .generated_files/flags/default/688cfdc9cc723357d8a31b3f7e0ca7f2ae5e5d54 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Protocol2.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Protocol2.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/Protocol2.o.d" -o ${OBJECTDIR}/_ext/1360937237/Protocol2.o ../src/Protocol2.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/Uart.o: ../src/Uart.c  .generated_files/flags/default/c7f72312d3fc709f8845d401f28f9d1e2959900d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/Uart.o.d" -o ${OBJECTDIR}/_ext/1360937237/Uart.o ../src/Uart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1360937237/Main.o: ../src/Main.c  .generated_files/flags/default/33611c53470a04d24cbff7424b18080d6b471e39 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/Main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -fno-common -I"../include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/Main.o.d" -o ${OBJECTDIR}/_ext/1360937237/Main.o ../src/Main.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}"  
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/Embedded_Firmware_Platform.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g -mdebugger -D__MPLAB_DEBUGGER_SIMULATOR=1 -mprocessor=$(MP_PROCESSOR_OPTION)  -o ${DISTDIR}/Embedded_Firmware_Platform.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)      -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--defsym=__MPLAB_DEBUGGER_SIMULATOR=1,--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}"
	
else
${DISTDIR}/Embedded_Firmware_Platform.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o ${DISTDIR}/Embedded_Firmware_Platform.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}"
	${MP_CC_DIR}/xc32-bin2hex ${DISTDIR}/Embedded_Firmware_Platform.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(wildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
