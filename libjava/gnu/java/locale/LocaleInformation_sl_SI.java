// This file was automatically generated by localedef.

package gnu.java.locale;

import java.util.ListResourceBundle;

public class LocaleInformation_sl_SI extends ListResourceBundle
{
  static final String decimalSeparator = ",";
  static final String groupingSeparator = " ";
  static final String numberFormat = "#.###";
  static final String percentFormat = "#%";
  static final String[] weekdays = { null, "nedelja", "ponedeljek", "torek", "sreda", "\u010Detrtek", "petek", "sobota" };

  static final String[] shortWeekdays = { null, "ned", "pon", "tor", "sre", "\u010Det", "pet", "sob" };

  static final String[] shortMonths = { "jan", "feb", "mar", "apr", "maj", "jun", "jul", "avg", "sep", "okt", "nov", "dec", null };

  static final String[] months = { "januar", "februar", "marec", "april", "maj", "junij", "julij", "avgust", "september", "oktober", "november", "december", null };

  static final String[] ampms = { "", "" };

  static final String shortDateFormat = "dd.MM.yyyy";
  static final String defaultTimeFormat = "";
  static final String currencySymbol = "SIT";
  static final String intlCurrencySymbol = "SIT";
  static final String currencyFormat = "#,###,##0.00 $;#,###,##0.00 $-";

  private static final Object[][] contents =
  {
    { "weekdays", weekdays },
    { "shortWeekdays", shortWeekdays },
    { "shortMonths", shortMonths },
    { "months", months },
    { "ampms", ampms },
    { "shortDateFormat", shortDateFormat },
    { "defaultTimeFormat", defaultTimeFormat },
    { "currencySymbol", currencySymbol },
    { "intlCurrencySymbol", intlCurrencySymbol },
    { "currencyFormat", currencyFormat },
    { "decimalSeparator", decimalSeparator },
    { "groupingSeparator", groupingSeparator },
    { "numberFormat", numberFormat },
    { "percentFormat", percentFormat },
  };

  public Object[][] getContents () { return contents; }
}