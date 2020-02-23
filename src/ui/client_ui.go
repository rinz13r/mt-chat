package main

/*
#cgo CFLAGS: -g -I ../../include/
int DEBUG;
#include "core/client.h"
#include "../ds/queue.c"
#include "../core/client.c"

int is_msg (void * resp) {
return (((struct ServerResponse*)resp)->type == MSG);
}
*/
import "C"
import (
	"fmt"
	"log"
	"time"
	"os"
	"strconv"
	"github.com/marcusolsson/tui-go"
)

func main() {
	if (len (os.Args) != 5) {
		fmt.Println (fmt.Sprintf ("Usage: %s <serv_ip> <port> <name> <channel>", os.Args[0]))
		os.Exit (1);
	}
	sidebar := tui.NewVBox(
		tui.NewLabel("CHANNELS"),
		tui.NewLabel("0"),
		tui.NewLabel("1"),
		tui.NewLabel("2"),
		// tui.NewLabel("DIRECT MESSAGES"),
		// tui.NewLabel("slackbot"),
		tui.NewSpacer(),
	)
	sidebar.SetBorder(true)

	history := tui.NewVBox()

	historyScroll := tui.NewScrollArea(history)
	historyScroll.SetAutoscrollToBottom(true)

	historyBox := tui.NewVBox(historyScroll)
	historyBox.SetBorder(true)

	input := tui.NewEntry()
	input.SetFocused(true)
	input.SetSizePolicy(tui.Expanding, tui.Maximum)

	inputBox := tui.NewHBox(input)
	inputBox.SetBorder(true)
	inputBox.SetSizePolicy(tui.Expanding, tui.Maximum)

	chat := tui.NewVBox(historyBox, inputBox)
	chat.SetSizePolicy(tui.Expanding, tui.Expanding)

	client := C.struct_Client {}
	serv_ip := C.CString (os.Args[1])
	i, err := strconv.Atoi (os.Args[2]);
	// port := 12;
	port := C.int (i);
	name := C.CString (os.Args[3]);
	i, err = strconv.Atoi (os.Args[4]);
	// channel := 12;
	channel := C.int (i);


	C.Client_init (&client, serv_ip, port, name, channel);



	username := os.Args[3];
	input.OnSubmit(func(e *tui.Entry) {
		C.Client_send (&client, C.CString (e.Text ()), 12)
		history.Append(tui.NewHBox(
			tui.NewLabel(time.Now().Format("15:04:05")),
			tui.NewPadder(1, 0, tui.NewLabel(fmt.Sprintf("<%s>", username))),
			tui.NewLabel(e.Text()),
			tui.NewSpacer(),
		))
		input.SetText("")
	})

	root := tui.NewHBox(sidebar, chat)

	ui, err := tui.New(root)
	if err != nil {
		log.Fatal(err)
	}

	f := func () {
		for (true) {
			resp := C.queue_pop (client.response_q);
			if (C.is_msg (resp) == 1) {
				msg := C.get_msg (resp);
				name := C.Msg_get_who (msg)
				history.Append(tui.NewHBox(
					tui.NewLabel(time.Now().Format("15:04:05")),
					tui.NewPadder(1, 0, tui.NewLabel(fmt.Sprintf("<%s>", C.GoString (name)))),
					tui.NewLabel(C.GoString (C.Msg_get_msg (msg))),
					tui.NewSpacer(),
					));
			} else {
				notif := C.get_notif (resp);
				msg := C.Notif_get_msg (notif);
				history.Append(tui.NewHBox(
					tui.NewLabel(time.Now().Format("15:04:05")),
					tui.NewPadder(1, 0, tui.NewLabel(fmt.Sprintf("[SERVER]"))),
					tui.NewLabel(C.GoString (msg)),
					tui.NewSpacer(),
					));
			}
			// ui.Painter.Repaint (ui.Root);
			// ui.Root
			ui.Repaint ()
		}
	};
	go f ();
	ui.SetKeybinding("Esc", func() { ui.Quit()
		C.Client_exit (&client);
	})

	if err := ui.Run(); err != nil {
		log.Fatal(err)
	}
	// C.Client_exit (&client);
}
