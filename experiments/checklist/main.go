// Adapted from https://github.com/dimakogan/checklist/blob/master/cmd/rpc_client/rpc_client.go
package main

import (
	"fmt"
	"math/rand"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"
	"time"

	"checklist/driver"
	"checklist/pir"
	"checklist/updatable"

	"log"
)

type requestTime struct {
	start, end time.Time
}

func main() {
  numberOfQueriesStr := os.Args[len(os.Args) - 1]
  numberOfQueries := 10000
  if strings.HasPrefix(numberOfQueriesStr, "q") {
    tmp, err := strconv.Atoi(numberOfQueriesStr[1:])
    if err != nil {
      log.Fatal("cannot parse number of queries: ", err)
    }
    numberOfQueries = tmp
  }
  fmt.Printf("Configured to make %d queries\n", numberOfQueries)

	config := new(driver.Config).AddPirFlags().AddClientFlags()
	latenciesFile := config.FlagSet.String("latenciesFile", "", "Latencies output filename")
	config.Parse()

	proxyLeft, err := config.ServerDriver()
	if err != nil {
		log.Fatal("Connection error: ", err)
	}

	proxyRight, err := config.Server2Driver()
	if err != nil {
		log.Fatal("Connection error: ", err)
	}

	fmt.Printf("Obtaining hint (this may take a while)...")
	client := updatable.NewClient(pir.RandSource(), config.PirType, [2]updatable.UpdatableServer{proxyLeft, proxyRight})
	client.CallAsync = true
	err = client.Init()
	if err != nil {
		log.Fatalf("Failed to Initialize client: %s\n", err)
	}
	fmt.Printf("[OK]\n")

	keys := client.Keys()
	fmt.Printf("Got %d keys from server\n", len(keys))

	inShutdown := false

	c := make(chan os.Signal)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-c
		inShutdown = true
	}()

	latencies := make(chan (requestTime), 1000)

	go func() {
		for i := 0; i < numberOfQueries; i++ {
			if inShutdown {
				break
			}
			key := keys[rand.Intn(len(keys))]
			start := time.Now()
			_, err := client.Read(key)
			if err != nil {
				fmt.Printf("Failed to read key %d: %v", key, err)
				continue
			}
			latencies <- requestTime{start, time.Now()}
		}
		
		close(latencies)
	}()

	var f *os.File
	if *latenciesFile != "" {
		f, err = os.OpenFile(*latenciesFile, os.O_TRUNC|os.O_CREATE|os.O_WRONLY, 0644)
		if err != nil {
			log.Fatalf("failed to create output file: %s", err)
		}
		fmt.Fprintf(f, "Seconds,Latency\n")
		defer f.Close()
	}

  totalTime := time.Since(time.Now())
	for l := range latencies {
	  totalTime += l.end.Sub(l.start)
		latency := l.end.Sub(l.start).Milliseconds()
		if f != nil {
			fmt.Fprintf(f, "%d,%d\n", l.start.Unix(), latency)
		}
	}
	fmt.Println(totalTime)

  // Print server times!
	var server1Time time.Duration
	var server2Time time.Duration
	
	proxyLeft.GetOnlineTimer(0, &server1Time)
	proxyRight.GetOnlineTimer(0, &server2Time)
	fmt.Printf("Server 1 time: %v\n", server1Time)
	fmt.Printf("Server 2 time: %v\n", server2Time)
}
