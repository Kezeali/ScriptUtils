################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/Engine.cpp \
../source/TypeTraits.cpp 

OBJS += \
./source/Engine.o \
./source/TypeTraits.o 

CPP_DEPS += \
./source/Engine.d \
./source/TypeTraits.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -DBOOST_BIND_NO_PLACEHOLDERS -I"/home/kezeali/FusionGit/ScriptUtils/include" -I"${BOOST_ROOT}/include" -O0 -g3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


