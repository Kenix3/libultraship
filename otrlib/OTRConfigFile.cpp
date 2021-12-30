#include "OTRConfigFile.h"
#include "spdlog/spdlog.h"
#include "OTRContext.h"
#include "OTRWindow.h"

namespace OtrLib {
	OTRConfigFile::OTRConfigFile(std::shared_ptr<OTRContext> Context, std::string Path) : Context(Context), Path(Path), File(Path.c_str()) {
		if (Path.empty()) {
			spdlog::error("OTRConfigFile received an empty file name");
			exit(EXIT_FAILURE);
		}

		if (!File.read(Val)) {
			if (!CreateDefaultConfig()) {
				spdlog::error("Failed to create default configs");
				exit(EXIT_FAILURE);
			}
		}
	}

	OTRConfigFile::~OTRConfigFile() {
		(*this)["WINDOW"]["FULLSCREEN"] = std::to_string(OTRContext::GetInstance()->GetWindow()->IsFullscreen());

		if (!Save()) {
			spdlog::error("Failed to save configs!!!");
		}
	}

	mINI::INIMap<std::string>& OTRConfigFile::operator[](std::string Section) {
		return Val[Section];
	}

	mINI::INIMap<std::string> OTRConfigFile::get(std::string Section) {
		return Val.get(Section);
	}

	bool OTRConfigFile::has(std::string Section) {
		return Val.has(Section);
	}

	bool OTRConfigFile::remove(std::string Section) {
		return Val.remove(Section);
	}

	void OTRConfigFile::clear() {
		Val.clear();
	}

	std::size_t OTRConfigFile::size() const {
		return Val.size();
	}

	bool OTRConfigFile::Save() {
		return File.write(Val);
	}

	bool OTRConfigFile::CreateDefaultConfig() {
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_CRIGHT)] = std::to_string(0x14D);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_CLEFT)] = std::to_string(0x14B);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_CDOWN)] = std::to_string(0x150);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_CUP)] = std::to_string(0x148);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_R)] = std::to_string(0x013);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_L)] = std::to_string(0x012);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_DRIGHT)] = std::to_string(0x023);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_DLEFT)] = std::to_string(0x021);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_DDOWN)] = std::to_string(0x022);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_DUP)] = std::to_string(0x014);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_START)] = std::to_string(0x039);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_Z)] = std::to_string(0x02C);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_B)] = std::to_string(0x02E);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_A)] = std::to_string(0x02D);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_STICKRIGHT)] = std::to_string(0x020);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_STICKLEFT)] = std::to_string(0x01E);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_STICKDOWN)] = std::to_string(0x01F);
		(*this)["KEYBOARD CONTROLLER 1"][STR(BTN_STICKUP)] = std::to_string(0x011);

		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_CRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_CLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_CDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_CUP)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_R)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_L)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_DRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_DLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_DDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_DUP)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_START)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_Z)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_B)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_A)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_STICKRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_STICKLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_STICKDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 2"][STR(BTN_STICKUP)] = "-1";

		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_CRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_CLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_CDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_CUP)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_R)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_L)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_DRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_DLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_DDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_DUP)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_START)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_Z)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_B)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_A)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_STICKRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_STICKLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_STICKDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 3"][STR(BTN_STICKUP)] = "-1";

		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_CRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_CLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_CDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_CUP)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_R)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_L)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_DRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_DLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_DDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_DUP)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_START)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_Z)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_B)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_A)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_STICKRIGHT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_STICKLEFT)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_STICKDOWN)] = "-1";
		(*this)["KEYBOARD CONTROLLER 4"][STR(BTN_STICKUP)] = "-1";

		(*this)["KEYBOARD SHORTCUTS"]["KEY_FULLSCREEN"] = std::to_string(0x044);
		(*this)["KEYBOARD SHORTCUTS"]["KEY_CONSOLE"] = std::to_string(0x029);

		(*this)["WINDOW"]["WINDOW WIDTH"] = std::to_string(320);
		(*this)["WINDOW"]["WINDOW HEIGHT"] = std::to_string(240);
		(*this)["WINDOW"]["FULLSCREEN WIDTH"] = std::to_string(1920);
		(*this)["WINDOW"]["FULLSCREEN HEIGHT"] = std::to_string(1080);
		(*this)["WINDOW"]["FULLSCREEN"] = std::to_string(false);

		(*this)["CONTROLLERS"]["CONTROLLER 1"] = "Keyboard";
		(*this)["CONTROLLERS"]["CONTROLLER 2"] = "Unplugged";
		(*this)["CONTROLLERS"]["CONTROLLER 3"] = "Unplugged";
		(*this)["CONTROLLERS"]["CONTROLLER 4"] = "Unplugged";

		return File.generate(Val);
	}
}