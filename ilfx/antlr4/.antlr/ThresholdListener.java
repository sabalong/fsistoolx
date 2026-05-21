// Generated from /Users/kennykarnama/fsistoolx/ilfx/antlr4/Threshold.g4 by ANTLR 4.13.1
import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link ThresholdParser}.
 */
public interface ThresholdListener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#ruleFile}.
	 * @param ctx the parse tree
	 */
	void enterRuleFile(ThresholdParser.RuleFileContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#ruleFile}.
	 * @param ctx the parse tree
	 */
	void exitRuleFile(ThresholdParser.RuleFileContext ctx);
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#ruleDecl}.
	 * @param ctx the parse tree
	 */
	void enterRuleDecl(ThresholdParser.RuleDeclContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#ruleDecl}.
	 * @param ctx the parse tree
	 */
	void exitRuleDecl(ThresholdParser.RuleDeclContext ctx);
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#rating}.
	 * @param ctx the parse tree
	 */
	void enterRating(ThresholdParser.RatingContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#rating}.
	 * @param ctx the parse tree
	 */
	void exitRating(ThresholdParser.RatingContext ctx);
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterExpr(ThresholdParser.ExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitExpr(ThresholdParser.ExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#orExpr}.
	 * @param ctx the parse tree
	 */
	void enterOrExpr(ThresholdParser.OrExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#orExpr}.
	 * @param ctx the parse tree
	 */
	void exitOrExpr(ThresholdParser.OrExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#andExpr}.
	 * @param ctx the parse tree
	 */
	void enterAndExpr(ThresholdParser.AndExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#andExpr}.
	 * @param ctx the parse tree
	 */
	void exitAndExpr(ThresholdParser.AndExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#comparison}.
	 * @param ctx the parse tree
	 */
	void enterComparison(ThresholdParser.ComparisonContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#comparison}.
	 * @param ctx the parse tree
	 */
	void exitComparison(ThresholdParser.ComparisonContext ctx);
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#value}.
	 * @param ctx the parse tree
	 */
	void enterValue(ThresholdParser.ValueContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#value}.
	 * @param ctx the parse tree
	 */
	void exitValue(ThresholdParser.ValueContext ctx);
	/**
	 * Enter a parse tree produced by {@link ThresholdParser#op}.
	 * @param ctx the parse tree
	 */
	void enterOp(ThresholdParser.OpContext ctx);
	/**
	 * Exit a parse tree produced by {@link ThresholdParser#op}.
	 * @param ctx the parse tree
	 */
	void exitOp(ThresholdParser.OpContext ctx);
}