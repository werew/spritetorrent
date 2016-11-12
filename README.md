# spritetorrent
Share hashes, share files, share everything...


## Functions and types:

### TLV (*Type Length Value*)

A TLV is the basic format when tranmitting data of any
kind. Here are the data structures to manipulate TLVs:

- **struct tlv**: 
  identifies every data of the form TLV (for example the messages)

- **tvlget\_length, tlvset\_length**: 
  used to read and write the length of a *struct tlv*

- **create_tlv, drop_tlv**: create and destroy a *struct tlv*


## Messages

Every time we must receive or send data we should do it trough a *struct msg*

- **struct msg**: contains all the infos about a message (received or to send),
    for instance:
    - *tlv*: a pointer to a *struct tlv* with the content of the message
    - *size* : the number of bytes readed (**NB:** this is different from
               the length inside the *struct tlv*, and sometime they could not
               match).
    - *addr*: a *struct sockaddr* containing all the infos about the source 
              address or the destination address (depending on the context)
    - *addrlen*: the size of the *struct sockaddr*


- **create_msg, drop_msg**: creates and drops a *struct msg*

- **accept_msg**: receive a message and returns a *struct msg* containing
                  the data and all the infos about the sender

- **send_msg**: sends the constent inside a *struct msg* (*tlv*) to
                the destination pointed by *addr* (**TODO**)



    

