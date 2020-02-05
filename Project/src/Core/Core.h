#pragma once
#include <stdint.h>
#include <iostream>

#define DECL_NO_COPY(Type) \
	Type(Type&&) = delete; \
	Type(const Type&) = delete; \
	Type& operator=(Type&&) = delete; \
	Type& operator=(const Type&) = delete

#define DECL_INTERFACE(Type) \
		Type() = default; \
		virtual ~Type() = default; \
		DECL_NO_COPY(Type)
	
#define SafeDelete(object) if (object) { delete object; object = nullptr; }