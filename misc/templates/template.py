# This is an example python actor
#
# Make sure the class name matches the filename! (without the .py file extension)

class actor(object):
    def __init__(self, *args, **kwargs):
        self.timeout = 1000         # Use this timeout value for when you need recurring handleTimer events
                                    # Set to -1 to wait infinite (default)

    def handleApi(self, command, *args, **kwargs):
        print("The API command is {} and its arguments is {}".format(command, args))
        return None

    def handleSocket(self, address, data, *args, **kwargs):
        print("The osc address is {} and its data is {}".format(address, data))
        return ("/myreturnaddress", ["hello", 3, 2, 1])

    def handleTimer(self, *args, **kwargs):
        # This is a timed event, use it as you need
        print("My timed event with type: {}, name: {}, uuid: {}".format(args[0], args[1], args[2]))
        return ("/mytimedreturn", ["hello", 1, 2, 3])

    def handleCustomSocket(self, *args, **kwargs):
        # We'll explain this in the future
        return ("/myreturnaddress", ["hello", "world"])

    def handleStop(self, *args, **kwargs):
        # We are shutting down
        print("Bye bye from {}".format(args[1]))
