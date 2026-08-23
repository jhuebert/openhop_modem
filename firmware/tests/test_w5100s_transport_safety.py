from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src" / "w5100s_ethernet_transport.cpp"
HTTP_SOURCE = ROOT / "src" / "w5100s_http_server.cpp"
BOUNDED_RX_SOURCE = ROOT / "src" / "w5100s_bounded_rx.cpp"


class W5100sTransportSafetyTest(unittest.TestCase):
    def test_tcp_protocol_send_path_is_cooperative_and_bounded(self) -> None:
        text = SOURCE.read_text(encoding="utf-8")
        self.assertNotIn("client.write(", text)
        self.assertIn("TX_DEADLINE_MS", text)
        self.assertIn("W5100.readSnTX_FSR(socket)", text)
        self.assertIn("W5100.writeSnCR(socket, Sock_SEND)", text)
        self.assertIn("SnIR::SEND_OK", text)
        self.assertIn("SnIR::TIMEOUT", text)
        self.assertIn("RX_BYTES_PER_LOOP", text)
        self.assertIn("socketReader.poll(millis(), byte)", text)
        self.assertGreaterEqual(text.count("if (client || txBusy()) disconnectClient();"), 2)

    def test_http_and_modem_receive_paths_bypass_unbounded_library_calls(self) -> None:
        modem = SOURCE.read_text(encoding="utf-8")
        http = HTTP_SOURCE.read_text(encoding="utf-8")
        bounded = BOUNDED_RX_SOURCE.read_text(encoding="utf-8")
        for source in (modem, http):
            self.assertNotIn("client.available()", source)
            self.assertNotIn("client.read()", source)
            self.assertNotIn("client.connected()", source)
            self.assertIn("socketReader.poll(millis(), byte)", source)
            self.assertIn("socketReader.settled(millis()", source)
        self.assertIn("STABLE_READ_ATTEMPTS", bounded)
        self.assertIn("COMMAND_DEADLINE_MS", bounded)
        self.assertIn("W5100.writeSnCR(socket_, Sock_RECV)", bounded)
        self.assertNotIn("execCmdSn", bounded)
        self.assertNotIn("while (", bounded)
        self.assertNotIn("ClientState::CLOSING", http)
        self.assertIn(
            "clearClientState(responseCompleted && socket < MAX_SOCK_NUM)", http
        )
        self.assertIn("W5100.writeSnCR(socket, Sock_CLOSE)", http)


if __name__ == "__main__":
    unittest.main()
