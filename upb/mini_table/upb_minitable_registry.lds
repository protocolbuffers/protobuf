/* Touch 17 */
SECTIONS
{
  upb_minitable_registry_section : ALIGN(8) {
    PROVIDE(__start_linkarr_upb_AllMiniTables = .);
    /* *(linkarr_upb_AllMiniTables) */
    KEEP(*(SORT_BY_NAME(linkarr_upb_AllMiniTables_h*)))
    PROVIDE(__stop_linkarr_upb_AllMiniTables = .);
  }

  upb_extension_registry_section : ALIGN(8) {
    PROVIDE(__start_linkarr_upb_AllExts = .);
    KEEP(*(linkarr_upb_AllExts))
    PROVIDE(__stop_linkarr_upb_AllExts = .);
  }
}
INSERT AFTER .rodata;
