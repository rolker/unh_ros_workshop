import hello_helpers.hello_misc as hm
## needs positon mode

class MyNode(hm.HelloNode):
    def __init__(self):
        hm.HelloNode.__init__(self)

    def main(self):
        hm.HelloNode.main(self, 'my_node', 'my_node', wait_for_first_pointcloud=False)

        # translate the base
        self.move_to_pose({'translate_mobile_base': 0.2}, blocking=True)

        # rotate the base
        self.move_to_pose({'rotate_mobile_base': 0.2}, blocking=True)

node = MyNode()
node.main()