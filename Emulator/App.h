#pragma once

#include <memory>

namespace guru
{
	class Debugger;

	class AmigaApp
	{
	public:
		explicit AmigaApp();
		~AmigaApp();

		bool Update();
		void Render();

		bool UseCrtEmulation() const { return m_useCrtEmulation; }

	private:
		bool m_isQuitting = false;
		bool m_useCrtEmulation = true;
		bool m_debuggerOpen = false;

		std::unique_ptr<Debugger> m_debugger;
	};
}
