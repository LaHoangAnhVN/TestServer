struct Request{
    enum Type{
        REQ_OPEN,
        REQ_UNLINK
    };
    Type reqquest_type;
    //int uid;
    char name[256];
};
