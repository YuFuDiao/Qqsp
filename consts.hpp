#ifndef QQSP_CONTST_HPP
#define QQSP_CONTST_HPP

#include <QLatin1StringView>

namespace Qqsp {
	constexpr QLatin1StringView logo{"Qt Quest Soft Player"};
	constexpr QLatin1StringView appname{"Qqsp"};
	constexpr QLatin1StringView config{"qqsp.ini"};
	constexpr QLatin1StringView customConfig{"custom.ini"};
	constexpr QLatin1StringView quicksave{"quicksave.sav"};
	constexpr QLatin1StringView game{"game.qsp"};

	namespace SettingsValues {
		namespace Application {
			constexpr QLatin1StringView langidSystem{"system"};
		}
	}
} // namespace Qqsp
#endif
