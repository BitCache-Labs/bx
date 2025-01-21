#pragma once

#define SINGLETON(Class)\
private:\
Class() = default;\
~Class() = default;\
Class(const Class&) = delete;\
Class& operator=(const Class&) = delete;\
Class(Class&&) = delete;\
Class& operator=(Class&&) = delete;