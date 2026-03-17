package main

import (
	"encoding/binary"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"time"
)

// ControllerState matches client state
type ControllerState struct {
	North        uint8 `json:"N"`
	East         uint8 `json:"E"`
	South        uint8 `json:"S"`
	West         uint8 `json:"W"`
	LeftBumper   uint8 `json:"LB"`
	RightBumper  uint8 `json:"RB"`
	LeftStick    uint8 `json:"LS"`
	RightStick   uint8 `json:"RS"`
	Select       uint8 `json:"SELECT"`
	Start        uint8 `json:"START"`
	LeftX        uint8 `json:"LjoyX"`
	LeftY        uint8 `json:"LjoyY"`
	RightX       uint8 `json:"RjoyX"`
	RightY       uint8 `json:"RjoyY"`
	LeftTrigger  uint8 `json:"LT"`
	RightTrigger uint8 `json:"RT"`
	DPadX        int8  `json:"dX"`
	DPadY        int8  `json:"dY"`
	Timestamp    int64 `json:"ts"`
}

// handleClient processes client connection
func handleClient(conn net.Conn) {
	defer conn.Close()

	log.Printf("Client connected: %s", conn.RemoteAddr())

	lastPrint := time.Now()

	for {
		// Read 4-byte length prefix
		hdr := make([]byte, 4)
		if _, err := io.ReadFull(conn, hdr); err != nil {
			if err == io.EOF {
				log.Printf("Client disconnected")
				return
			}
			log.Printf("Read header error: %v", err)
			return
		}
		totalLen := binary.BigEndian.Uint32(hdr)
		if totalLen == 0 {
			log.Printf("Zero-length packet, skipping")
			continue
		}
		if totalLen > uint32(MaxPacketSize+4) { // payload + crc shouldn't exceed MaxPacketSize+4
			log.Printf("Packet too large: %d bytes (max %d)", totalLen, MaxPacketSize+4)
			// Drain and continue (attempt to read and discard)
			if _, err := io.CopyN(io.Discard, conn, int64(totalLen)); err != nil {
				log.Printf("drain error: %v", err)
				return
			}
			continue
		}

		buf := make([]byte, totalLen)
		if _, err := io.ReadFull(conn, buf); err != nil {
			log.Printf("Read packet error: %v", err)
			return
		}

		payload, ok := VerifyPacket(buf)
		if !ok {
			log.Printf("CRC mismatch from %s, dropping packet", conn.RemoteAddr())
			continue
		}

		var state ControllerState
		if err := json.Unmarshal(payload, &state); err != nil {
			log.Printf("JSON unmarshal error: %v", err)
			continue
		}

		// Debug print every second
		if time.Since(lastPrint) > time.Second {
			fmt.Printf("State: %v\n", &state)
			lastPrint = time.Now()
		}

		// TODO: Forward validated JSON to C gatekeeper process
	}
}

func main() {
	port := flag.Int("port", 8080, "Server port")
	public := flag.Bool("public", false, "Allow external connections")
	flag.Parse()

	// Setup listener
	addr := fmt.Sprintf("localhost:%d", *port)
	if *public {
		addr = fmt.Sprintf("0.0.0.0:%d", *port)
	}

	listener, err := net.Listen("tcp", addr)
	if err != nil {
		log.Fatal(err)
	}
	defer listener.Close()

	log.Printf("Server listening on %s", addr)

	// Accept connections
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Printf("Accept error: %v", err)
			continue
		}

		go handleClient(conn)
	}
}

const MaxPacketSize = 1024

func VerifyPacket(buf []byte) ([]byte, bool) {
	if len(buf) > 4 {
		return buf[:len(buf)-4], true
	}
	return buf, true
}
func AppendCRC(data []byte) []byte {
	return data
}
