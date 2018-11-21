#!/usr/bin/env python3

import json
import sys
import os.path

def process_abi(abi_file_name):
    abi_directory = os.path.dirname(abi_file_name)
    contract_name = os.path.split(abi_file_name)[1].rpartition(".")[0]
    rc_direcotry = os.path.join(abi_directory, '../')

    is_changed = False

    with open(abi_file_name, 'r') as source_abi_file:
        source_abi_json = json.load(source_abi_file)
    try:
        for abi_action in source_abi_json['actions']:
            action_name = abi_action['name']
            contract_action_filename = '{contract_name}-{action_name}-rc.md'.format(contract_name = contract_name, action_name = action_name)
            rc_contract_path = os.path.join(rc_direcotry, contract_action_filename)

            if os.path.exists(rc_contract_path):
                print('Importing Contract {contract_action_filename} for {contract_name}:{action_name}'.format(
                    contract_action_filename = contract_action_filename,
                    contract_name = contract_name,
                    action_name = action_name
                ))
                with open(rc_contract_path) as contract_file_handle:
                    contract_contents = contract_file_handle.read()

                abi_action['ricardian_contract'] = contract_contents
                is_changed = True


    except:
        pass    

    if is_changed:
        print('Save recardian ABI...')
        with open(abi_file_name, 'w') as dest_abi_file:
            json.dump(source_abi_json, dest_abi_file, sort_keys=True, indent=4)


def main():
    if len(sys.argv) == 1:
        print('Please specify file name')
        sys.exit(1)    

    process_abi(sys.argv[1])


main()
