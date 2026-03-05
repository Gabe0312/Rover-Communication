// package main 

// import (
// 	"bufio"
// 	"fmt"
// 	"log"
// 	"os"
// 	"os/signal"
// 	"syscall"
// 	"time"

// 	"go.bug.st/serial"

// )

// func main() {

// 	mode := &serial.Mode{BaudRate: 9600,}

// 	piSerialPort, err := serial.Open("/dev/serial0", mode)

// 	if err != nil {
// 		log.Fatal(err)
// 	}

// 	defer piSerialPort.Close()

// 	c := make(chan os.Signal, 1)

// 	signal.Notify(c, os.Interrupt, syscall.SIGTERM)

// 	go func() {
// 		<-c		
// 		piSerialPort.Close()
// 		fmt.Println("\nReceived interrupt signal, exiting...")
// 		os.Exit(0)
// 	}()

// 	scanner := bufio.NewScanner(piSerialPort)

// 	for{

// 		if scanner.Scan() {
// 			dataRead := scanner.Text()
// 			fmt.Println("Data received:", dataRead)
// 		}

// 		if err := scanner.Err(); err != nil {
// 			log.Println("Error reading from serial port:", err)
// 		}

// 		time.Sleep(100 * time.Millisecond)
// 	}



// }
//