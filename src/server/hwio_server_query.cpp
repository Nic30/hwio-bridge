#include "hwio_server.h"

using namespace std;
using namespace hwio;

vector<hwio_comp_spec> HwioServer::extractSpecFromClientQuery(DevQuery * q,
		int cnt) {
	vector<hwio_comp_spec> spec;

	for (Dev_query_item * item = q->items; item < q->items + cnt; ++item) {
		char * name = item->device_name;
		char * vendor = item->vendor_name;
		char * type = item->type_name;

		spec.push_back( { vendor, type, item->version });
		spec.back().name_set(name);
	}

	return spec;
}

void register_device_for_client() {

}

HwioServer::PProcRes HwioServer::device_lookup_resp(ClientInfo * client,
		DevQuery * q, int cnt, char tx_buffer[BUFFER_SIZE]) {
	vector<hwio_comp_spec> spec = extractSpecFromClientQuery(q, cnt);
	HwioFrame<DevQueryResp> * resp =
			reinterpret_cast<HwioFrame<DevQueryResp>*>(tx_buffer);
	resp->header.command = HWIO_CMD_QUERY_RESP;
//         std::cerr << cnt << std::endl;
//         std::cerr << q->items[0].device_name << std::endl;
//         std::cerr << q->items[0].vendor_name << std::endl;
//         std::cerr << q->items[0].type_name << std::endl;
//         std::cerr << q->items[0].version << std::endl;
        
	vector<ihwio_dev *> result;
	for (auto & bus : buses) {
		for (auto dev : bus->find_devices(spec)) {
			result.push_back(dev);
		}
	}
// 	std::cerr << result.size() << std::endl;
	int i = 0;
	resp->header.body_len = 0;
	for (auto dev : result) {
		dev_id_t dev_id = 0;
		int firts_empty = -1;
		bool device_already_registered = false;
		for (auto cdev : client->devices) {
			if (cdev == dev) {
				device_already_registered = true;
				break;
			}

			if (cdev == nullptr && firts_empty < 0) {
				firts_empty = dev_id;
				// we can not break because we need to search
				// if divece is already in client devices
			}

			dev_id++;
		}
		if (!device_already_registered) {
			if (firts_empty < 0) {
				assert(client->devices.size() == dev_id);
				client->devices.push_back(dev);
			} else {
				client->devices[firts_empty] = dev;
				dev_id = firts_empty;
			}
			dev->attach();
		}

		resp->header.body_len += sizeof(dev_id_t);
		resp->body.ids[i] = dev_id;
		i++;

		if (log_level >= logDEBUG) {
			std::cout << "[DEBUG] Client " << client->id << " now owns "
					<< (int) dev_id << endl;
			std::cout << dev->to_str() << std::endl;
		}
	}

	return PProcRes(false, sizeof(Hwio_packet_header) + resp->header.body_len);
}
