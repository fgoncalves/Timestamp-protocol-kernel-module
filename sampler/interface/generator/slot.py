class Slot:

    def __init__(self, source):
        self.source = source
        self.packet_delay_data = list()
        self.packet_data = list()

    def __str__(self):
        return self.source + "\n\t" + str(self.packet_delay_data) + "\n\t" + str(self.packet_delay_variation_data) + "\n\t" + str(self.packet_data)
