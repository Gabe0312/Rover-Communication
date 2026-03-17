package main

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"net"
	"time"

	"github.com/0xcafed00d/joystick"
)

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

func main() {
	fmt.Println("Base Station Initialized")
    
	conn, err := net.Dial("tcp", "192.168.4.214:8080")
    

	if err != nil{
		fmt.Println("connection not found:", err)
		return 
	}
	defer conn.Close()
	js, err := joystick.Open(0)

	if err != nil {
		fmt.Println("Controller not found:", err)
		return
	}

	state := ControllerState{
		LeftX: 128,
		LeftY: 200,
	}
	
	for {
		joyState, err := js.Read()
		if err != nil {
			fmt.Println("Error reading controller:", err)
			return
		}
		if len(joyState.AxisData) < 4{
			fmt.Println("Axis length has less than 4 entries")
			time.Sleep(100 * time.Millisecond)
			continue
		}

		rawLeftY := joyState.AxisData[1]
		rawRightY := joyState.AxisData[3]

		leftSpeed := uint8((rawLeftY + 32768) / 256)
	    rightSpeed := uint8((rawRightY + 32768) / 256)

		state.LeftY = leftSpeed
		state.RightY = rightSpeed

		data, err := json.Marshal(state) // transtales what we have in the state instance into bytes by first turning it into a string.
		
		if err != nil {
			fmt.Println("marshal error:", err)
			return
	    }
		
		fmt.Println(string(data))

		fmt.Println("Axis Data:", joyState.AxisData)
		
		tail := []byte{0,0,0,0}

	    totalLength := uint32(len(data) + 4)

	    header := make([]byte, 4)

	    binary.BigEndian.PutUint32(header, totalLength)

	    _, err = conn.Write(header)

		if err != nil {
			fmt.Println("Connection lost:header",err)
			return
		}
	    _, err = conn.Write(data)
		if err != nil {
			fmt.Println("Connection lost:data",err)
			return
		}
	    _, err = conn.Write(tail)
		if err != nil {
			fmt.Println("Connection lost:tail",err)
			return
		}

		time.Sleep(50 * time.Millisecond)
	}
	fmt.Println("Rover connection successful")
}
