#define CODE                                                                                                           \
    "adox r10, r14;\n"                                                                                                 \
    "adox r10, r9;\n"                                                                                                  \
    "adox r10, rcx;\n"                                                                                                 \
    "adox r11, r12;\n"                                                                                                 \
    "adox r11, r15;\n"                                                                                                 \
    "adox r11, r9;\n"                                                                                                  \
    "adox r11, rbx;\n"                                                                                                 \
    "adox r12, r10;\n"                                                                                                 \
    "adox r12, r13;\n"                                                                                                 \
    "adox r12, r15;\n"                                                                                                 \
    "adox r12, rbp;\n"                                                                                                 \
    "adox r13, r11;\n"                                                                                                 \
    "adox r13, r12;\n"                                                                                                 \
    "adox r13, r15;\n"                                                                                                 \
    "adox r13, rbp;\n"                                                                                                 \
    "adox r13, [ rsp + 0x68 ];\n"                                                                                      \
    "adox r14, r10;\n"                                                                                                 \
    "adox r14, r15;\n"                                                                                                 \
    "adox r14, rax;\n"                                                                                                 \
    "adox r14, rcx;\n"                                                                                                 \
    "adox r14, rdi;\n"                                                                                                 \
    "adox r15, r11;\n"                                                                                                 \
    "adox r15, r13;\n"                                                                                                 \
    "adox r15, r8;\n"                                                                                                  \
    "adox r15, rdx;\n"                                                                                                 \
    "adox r8, r10;\n"                                                                                                  \
    "adox r8, r13;\n"                                                                                                  \
    "adox r8, r9;\n"                                                                                                   \
    "adox r8, rbp;\n"                                                                                                  \
    "adox r8, rsi;\n"                                                                                                  \
    "adox r9, r11;\n"                                                                                                  \
    "adox r9, r12;\n"                                                                                                  \
    "adox r9, r13;\n"                                                                                                  \
    "adox r9, r15;\n"                                                                                                  \
    "adox r9, rax;\n"                                                                                                  \
    "adox r9, rcx;\n"                                                                                                  \
    "adox r9, [ rsp + 0x28 ];\n"                                                                                       \
    "adox rax, r10;\n"                                                                                                 \
    "adox rax, r12;\n"                                                                                                 \
    "adox rax, r15;\n"                                                                                                 \
    "adox rax, r8;\n"                                                                                                  \
    "adox rax, rbp;\n"                                                                                                 \
    "adox rbp, rax;\n"                                                                                                 \
    "adox rbp, [ rsp + 0x10 ];\n"                                                                                      \
    "adox rbx, r11;\n"                                                                                                 \
    "adox rbx, r8;\n"                                                                                                  \
    "adox rbx, r9;\n"                                                                                                  \
    "adox rbx, r9;\n"                                                                                                  \
    "adox rbx, rax;\n"                                                                                                 \
    "adox rbx, rdi;\n"                                                                                                 \
    "adox rcx, r12;\n"                                                                                                 \
    "adox rcx, r9;\n"                                                                                                  \
    "adox rcx, rax;\n"                                                                                                 \
    "adox rcx, rbp;\n"                                                                                                 \
    "adox rcx, rbp;\n"                                                                                                 \
    "adox rdi, r13;\n"                                                                                                 \
    "adox rdi, rax;\n"                                                                                                 \
    "adox rdi, rbx;\n"                                                                                                 \
    "adox rdi, rcx;\n"                                                                                                 \
    "adox rdx, r8;\n"                                                                                                  \
    "adox rdx, r9;\n"                                                                                                  \
    "adox rdx, rax;\n"                                                                                                 \
    "adox rdx, rcx;\n"                                                                                                 \
    "adox rdx, rsi;\n"                                                                                                 \
    "adox rsi, rdi;\n"                                                                                                 \
    ""
