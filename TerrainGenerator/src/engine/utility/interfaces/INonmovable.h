#pragma once

class INonmovable
{
protected:
	INonmovable() = default;

	INonmovable(INonmovable&&) noexcept = delete;
	INonmovable& operator =(INonmovable&&) noexcept = delete;

	~INonmovable() noexcept = default;
};