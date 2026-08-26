from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
CONFIG_SOURCE = ROOT / "src" / "rak4631_config.cpp"
BOOTLOADER_SOURCE = ROOT / "src" / "bootloader_manager.cpp"
WEBUI_SOURCE = ROOT / "src" / "webui_shared.cpp"


class Rak4631PersistenceSafetyTest(unittest.TestCase):
    def test_softdevice_is_disabled_before_usb_and_runtime_flash_writes(self) -> None:
        header = (ROOT / "include" / "rak4631_config.h").read_text(encoding="utf-8")
        main = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        config = CONFIG_SOURCE.read_text(encoding="utf-8")
        self.assertIn("bool prepareFlashRuntime();", header)
        self.assertIn("Rak4631Config::prepareFlashRuntime();", main)
        self.assertLess(
            main.index("Rak4631Config::prepareFlashRuntime();"),
            main.index("Serial.begin(921600);"),
        )
        self.assertIn("sd_softdevice_disable()", config)
        save = config[config.index("bool saveConfig("):config.index("bool factoryReset(")]
        self.assertIn("flashWritesAreSynchronous()", save)

    def test_save_uses_bounded_dual_slot_store_not_littlefs(self) -> None:
        text = CONFIG_SOURCE.read_text(encoding="utf-8")
        save = text[text.index("bool saveConfig("):text.index("bool factoryReset(")]
        store_header = (ROOT / "include" / "rak4631_config_store.h").read_text(
            encoding="utf-8"
        )
        self.assertIn("configStore.save(normalized)", save)
        self.assertNotIn("InternalFS", save)
        self.assertIn("SLOT_A_ADDRESS = CONFIG_REGION_START", store_header)
        self.assertIn("SLOT_B_ADDRESS = CONFIG_REGION_START + 0x1000u", store_header)
        self.assertIn("CONFIG_REGION_START = 0xED000u", store_header)
        self.assertIn("CONFIG_REGION_END = 0xF4000u", store_header)

    def test_save_path_does_not_block_on_usb_serial_logging(self) -> None:
        text = CONFIG_SOURCE.read_text(encoding="utf-8")
        save = text[text.index("bool saveConfig("):text.index("bool factoryReset(")]
        self.assertNotIn("Serial.", save)

    def test_runtime_flash_uses_synchronous_nvmc_not_softdevice_cache(self) -> None:
        text = CONFIG_SOURCE.read_text(encoding="utf-8")
        flash = text[text.index("class NrfConfigFlash"):text.index("NrfConfigFlash configFlash")]
        self.assertNotIn("flash_nrf5x_", flash)
        self.assertIn("nrf_nvmc_page_erase_start", flash)
        self.assertIn("nrf_nvmc_mode_set", flash)
        self.assertIn("nrf_nvmc_ready_check", flash)

    def test_reboot_and_dfu_do_not_touch_internal_filesystem(self) -> None:
        bootloader = BOOTLOADER_SOURCE.read_text(encoding="utf-8")
        execute = bootloader[bootloader.index("void execute("):]
        self.assertNotIn("InternalFS", execute)
        self.assertNotIn("Rak4631Config", execute)

    def test_integer_rendering_does_not_depend_on_long_long_printf(self) -> None:
        text = WEBUI_SOURCE.read_text(encoding="utf-8")
        number = text[text.index("template <typename T>"):text.index("std::string fixed(")]
        self.assertNotIn("snprintf", number)
        self.assertNotIn("%ll", number)

    def test_dfu_address_matches_bootloader_increment_and_is_labeled_as_dfu(self) -> None:
        config = CONFIG_SOURCE.read_text(encoding="utf-8")
        address = config[config.index("const char* getDfuBluetoothAddress()") :]
        webui = WEBUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("(low & 0xffU) + 1U", address)
        self.assertIn("Expected <code>4631_DFU</code> address", webui)
        self.assertIn("different from the application address", webui)


if __name__ == "__main__":
    unittest.main()
