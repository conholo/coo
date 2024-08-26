#pragma once

class RenderPassResource
{
public:
	explicit RenderPassResource(std::string name)
		: m_Name(std::move(name)) {}
	virtual ~RenderPassResource() = default;

	const std::string& GetName() const { return m_Name; }

private:
	std::string m_Name;
};
