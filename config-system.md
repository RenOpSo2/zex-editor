ConfigSystem
├── load(sources: [File, CLI, Default])
│   └── Baca ~/.zexrc atau ./zex.json + args
│
├── get(key: string) → any
│   └── get("tabsize") → 2
│   └── get("mouse") → true
│
├── set(key: string, value: any) → void
│   └── set("tabsize", 4) → update file
│
├── watch() → EventEmitter
│   └── Reload config kalo file berubah
│
└── validate(raw: any) → SchemaError[]
    └── Cek tabsize must be number, mouse must be boolean
