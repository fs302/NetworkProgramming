Simulating TCP congestion control Using UDP

[Sender]

sender_Stable.c: constantly sending MAX_WINDOW_SIZE packets to the receiver.

sender_SlowStart.c: start from sending 1 packet, and double the cwnd for each RTT when lose packets not occur, clear the cwnd=1 when lose packets happend.

sender_AIMD.c: start from sending 1 packet, adding 1 to cwnd for each RTT when lose packets not occur, half the cwnd when lose packets happend.

[Receiver]

receiver.c: Send file request and receive file packets.

[Middle]

middle.c: Receive packets from sender and decides whether or not send it to the Receiver according to the bandwidth limit.
