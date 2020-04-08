#pragma once

class INoncopyable
{
protected:
	INoncopyable() = default;
	
	INoncopyable(const INoncopyable&) = delete;
	INoncopyable& operator =(const INoncopyable&) = delete;
	
	~INoncopyable() noexcept = default;
};