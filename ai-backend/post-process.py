import re

pattern_ice = re.compile(r"\s\s\s`Implicit Cast\(LValueToRValue\)`\n\s+- 类型: `外部类型（隐式转换）`\n\s+- 定义路径: `(.*?)`\n\s+- 简介：`<Filled-By-AI>`\n\s+- 外部类型细节：\n\s+  - 外部类型名称: `(.*?)`\n\s+     - 位置: `(.*?)`\n\s+     - 是否为指针: `(\S*)`\n\n")

target_text = """
   `cond_resched`
   - 类型: `宏`
   - 定义路径: `./include/linux/sched.h:2097`
   - 简介：`<Filled-By-AI>`

   `void anon_vma_unlock_read(struct anon_vma * anon_vma)`
   - 类型: `函数`
   - 定义路径: `./include/linux/rmap.h:139`
   - 简介：`<Filled-By-AI>`
 """

pattern_basic_ability = re.compile(r'\s\s\s`(.*?)`\n\s+- 类型: `(.*?)`\n\s+- 定义路径: `(.*?)`\n\s+- 简介：`<Filled-By-AI>`\n(?:\s+- 外部类型细节：\n\s+-(?:.*?\n\s+)*?.*?位置: `(.*?)`\n\s+- 是否为指针: `(.*?)`\n)?')

pattern_basic_ability = re.compile(r'\s\s\s`(.*?)`\n\s+- 类型: `(.*?)`\n\s+- 定义路径: `(.*?)`\n\s+- 简介：`<Filled-By-AI>`\n(?:\s+- 外部类型细节：\n\s+- 外部类型名称: `(.*?)`\n\s+- 位置: `(.*?)`\n\s+- 是否为指针: `(.*?)`\n)?')

def count_subentries(file_path):
    subentry_counts = {}
    in_function_entry = False

    with open(file_path, 'r', encoding='utf-8') as file:
        file_content = file.read()
        matches = pattern_basic_ability.findall(file_content)

        for match in matches:
            entry_name = match[0]
            entry_type = match[1]
            entry_path = match[2]
            # print(match)

            if re.search(r'外部类型.*?（函数参数|变量声明）', entry_type):
                entry_type = "外部类型"
                entry_path = match[4]
                struct_match = re.search(r'struct\s+(\S+)', entry_name)
                if struct_match:
                    entry_name = "struct" + ' ' + struct_match.group(1)

            subentry = f"   `{entry_name}`\n   - 类型: `{entry_type}`\n   - 定义路径: `{entry_path}`\n   - 简介：`<Filled-By-AI>`\n"
            subentry_counts[subentry] = subentry_counts.get(subentry, 0) + 1

    return subentry_counts

def format_and_write_output(counts, count_result_file):
    with open(count_result_file, 'w', encoding='utf-8') as count_result:
        # count_result.write("出现超过0次的子条目:\n")
        # cnt = 0
        # for k, v in counts.items():
        #     if v > 0:
        #         cnt += 1
        #         count_result.write(f"{k}: {v}次\n")
        # count_result.write(f"共{cnt}个\n")

        count_result.write("出现超过3次的子条目:\n")
        cnt = 0
        for k, v in counts.items():
            if v > 3:
                cnt += 1
                count_result.write(f"{k}: {v}次\n")
        count_result.write(f"共{cnt}个\n")

        count_result.write("\n出现超过5次的子条目（基础能力Threshold）:\n")
        for k, v in counts.items():
            if v > 5:
                count_result.write(f"{k}\n")

        count_result.write("\n出现超过10次的子条目:\n")
        for k, v in counts.items():
            if v > 10:
                count_result.write(f"{k}: {v}次\n")

def rewrite_file(count_map, file_path, post_processed_file):
    with open(file_path, 'r', encoding='utf-8') as file:
        file_content = file.read()
        matches = pattern_basic_ability.findall(file_content)

        for match in matches:
            entry_name = match[0]
            entry_type = match[1]
            entry_path = match[2]
            # print(match)

            if re.search(r'外部类型.*?（函数参数|变量声明）', entry_type):
                entry_type = "外部类型"
                entry_path = match[4]
                struct_match = re.search(r'struct\s+(\S+)', entry_name)
                if struct_match:
                    entry_name = "struct" + ' ' + struct_match.group(1)

            subentry = f"   `{entry_name}`\n   - 类型: `{entry_type}`\n   - 定义路径: `{entry_path}`\n   - 简介：`<Filled-By-AI>`\n"

            if subentry in count_map and count_map[subentry] > 5:
                original_text = ""
                new_text = f"   `{match[0]}`\n   - 是否为基础能力: `是`\n"
                if match[4] == "":
                    original_text = f"   `{match[0]}`\n   - 类型: `{match[1]}`\n   - 定义路径: `{match[2]}`\n   - 简介：`<Filled-By-AI>`\n"
                else:
                    original_text = f"   `{match[0]}`\n   - 类型: `{match[1]}`\n   - 定义路径: `{match[2]}`\n   - 简介：`<Filled-By-AI>`\n   - 外部类型细节：\n      - 外部类型名称: `{match[3]}`\n         - 位置: `{match[4]}`\n         - 是否为指针: `{match[5]}`\n"
                # write to post_processed_file: replace original_text with new_text
                # print(original_text)
                file_content = file_content.replace(original_text, new_text)
                
        # Write to post_processed_file
        with open(post_processed_file, 'w', encoding='utf-8') as post_processed:
            post_processed.write(file_content)



def main():
    file_path = '/home/tz/MigrationHint/log/download/ksm_result.md'
    count_result_file = '统计输出新.txt'
    post_processed_file = 'post_processed.md'
    counts = count_subentries(file_path)
    format_and_write_output(counts, count_result_file)
    rewrite_file(counts, file_path, post_processed_file)

if __name__ == "__main__":
    main()
