module.exports = [
  {
    "type": "heading",
    "defaultValue": "Nooka"
  },

  // ── Colors ──────────────────────────────────────────────────────────────────
  {
    "type": "color",
    "allowGray": true,
    "messageKey": "BG_COLOR",
    "label": "Background",
    "defaultValue": "FFFFFF"
  },
  {
    "type": "color",
    "allowGray": true,
    "messageKey": "FILLED_COLOR",
    "label": "Active (dots & bar)",
    "defaultValue": "555555"
  },
  {
    "type": "color",
    "allowGray": true,
    "messageKey": "EMPTY_COLOR",
    "label": "Inactive (unfilled)",
    "defaultValue": "AAAAAA"
  },

  // ── Display ──────────────────────────────────────────────────────────────────
  {
    "type": "heading",
    "defaultValue": "Display"
  },
  {
    "type": "toggle",
    "messageKey": "SHOW_DATE",
    "label": "Show Date (bottom left)",
    "defaultValue": true
  },
  {
    "type": "select",
    "messageKey": "DATE_STYLE",
    "label": "Date Format",
    "defaultValue": "0",
    "options": [
      { "label": "MM/DD/YY", "value": "0" },
      { "label": "MM/DD",    "value": "1" },
      { "label": "DD/MM/YY", "value": "2" },
      { "label": "DD/MM",    "value": "3" }
    ]
  },

  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
